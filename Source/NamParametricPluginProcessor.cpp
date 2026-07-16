#include "NamParametricPluginProcessor.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "NamParametricPluginEditor.h"

namespace {
constexpr const char* kModelPathStateKey = "modelPath";
constexpr const char* kRuntimeParamsStateType = "RUNTIME_PARAMS";
constexpr const char* kRuntimeParamStateType = "PARAM";
constexpr const char* kRuntimeParamIndexStateKey = "index";
constexpr const char* kRuntimeParamNameStateKey = "name";
constexpr const char* kRuntimeParamValueStateKey = "value";
constexpr int kMinimumModelResetBlockSize = 2048;

std::vector<NamParametricPluginAudioProcessor::RuntimeParameterInfo> ConvertRuntimeParameters(
    const std::vector<namparametric::dsp::ModelParameterInfo>& params) {
  std::vector<NamParametricPluginAudioProcessor::RuntimeParameterInfo> out;
  out.reserve(params.size());

  for (const auto& param : params) {
    NamParametricPluginAudioProcessor::RuntimeParameterInfo info;
    info.name = param.name;
    info.defaultValue = param.defaultValue;
    info.minValue = param.minValue;
    info.maxValue = param.maxValue;
    info.enumNames.reserve(param.enumNames.size());
    for (const auto& enumName : param.enumNames) {
      info.enumNames.emplace_back(enumName);
    }
    out.push_back(std::move(info));
  }

  return out;
}

juce::ValueTree CreateRuntimeParameterState(
    const std::vector<NamParametricPluginAudioProcessor::RuntimeParameterInfo>& params,
    const std::vector<float>& values) {
  juce::ValueTree runtimeTree(kRuntimeParamsStateType);
  const size_t count = std::min(params.size(), values.size());
  for (size_t i = 0; i < count; ++i) {
    juce::ValueTree paramTree(kRuntimeParamStateType);
    paramTree.setProperty(kRuntimeParamIndexStateKey, static_cast<int>(i), nullptr);
    paramTree.setProperty(kRuntimeParamNameStateKey, params[i].name, nullptr);
    paramTree.setProperty(kRuntimeParamValueStateKey, values[i], nullptr);
    runtimeTree.addChild(paramTree, -1, nullptr);
  }
  return runtimeTree;
}
}  // namespace

NamParametricPluginAudioProcessor::RuntimeParameterMailbox::RuntimeParameterMailbox(
    const std::span<const float> initialValues)
    : mSize(initialValues.size()),
      mValues(mSize == 0 ? nullptr : std::make_unique<std::atomic<float>[]>(mSize)) {
  for (size_t i = 0; i < mSize; ++i) {
    mValues[i].store(initialValues[i], std::memory_order_relaxed);
  }
}

void NamParametricPluginAudioProcessor::RuntimeParameterMailbox::Publish(const size_t index,
                                                                         const float value) {
  if (index >= mSize) {
    return;
  }

  std::lock_guard<std::mutex> lock(mWriterMutex);
  mVersion.fetch_add(1, std::memory_order_acq_rel);
  mValues[index].store(value, std::memory_order_relaxed);
  mVersion.fetch_add(1, std::memory_order_release);
}

void NamParametricPluginAudioProcessor::RuntimeParameterMailbox::PublishAll(
    const std::span<const float> values) {
  if (values.size() != mSize) {
    return;
  }

  std::lock_guard<std::mutex> lock(mWriterMutex);
  mVersion.fetch_add(1, std::memory_order_acq_rel);
  for (size_t i = 0; i < mSize; ++i) {
    mValues[i].store(values[i], std::memory_order_relaxed);
  }
  mVersion.fetch_add(1, std::memory_order_release);
}

std::optional<float> NamParametricPluginAudioProcessor::RuntimeParameterMailbox::Read(
    const size_t index) const {
  if (index >= mSize) {
    return std::nullopt;
  }
  return mValues[index].load(std::memory_order_acquire);
}

bool NamParametricPluginAudioProcessor::RuntimeParameterMailbox::TryReadStable(
    const std::span<float> destination, uint64_t& version) const {
  if (destination.size() != mSize) {
    return false;
  }

  constexpr int kMaxAttempts = 3;
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    const uint64_t versionBefore = mVersion.load(std::memory_order_acquire);
    if ((versionBefore & 1ULL) != 0) {
      continue;
    }

    for (size_t i = 0; i < mSize; ++i) {
      destination[i] = mValues[i].load(std::memory_order_relaxed);
    }

    const uint64_t versionAfter = mVersion.load(std::memory_order_acquire);
    if (versionBefore == versionAfter && (versionAfter & 1ULL) == 0) {
      version = versionAfter;
      return true;
    }
  }

  return false;
}

NamParametricPluginAudioProcessor::NamParametricPluginAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::mono(), true)
                               .withOutput("Output", juce::AudioChannelSet::mono(), true)),
      mValueTree(*this, nullptr, "PARAMS", CreateParameterLayout()) {
  mInputGainParam = mValueTree.getRawParameterValue(ParamIDs::inputGainDb);
  mOutputGainParam = mValueTree.getRawParameterValue(ParamIDs::outputGainDb);
  startTimerHz(30);
}

NamParametricPluginAudioProcessor::~NamParametricPluginAudioProcessor() { stopTimer(); }

void NamParametricPluginAudioProcessor::prepareToPlay(const double sampleRate,
                                                      const int samplesPerBlock) {
  const int preparedBlockSize = std::max(1, samplesPerBlock);
  mHostProcessingConfig->generation.fetch_add(1, std::memory_order_acq_rel);
  mHostProcessingConfig->sampleRate.store(sampleRate, std::memory_order_relaxed);
  mHostProcessingConfig->blockSize.store(preparedBlockSize, std::memory_order_relaxed);
  mHostProcessingConfig->generation.fetch_add(1, std::memory_order_release);

  mInputScratch.assign(static_cast<size_t>(preparedBlockSize), 0.0f);
  mOutputScratch.assign(static_cast<size_t>(preparedBlockSize), 0.0f);

  std::lock_guard<std::mutex> lock(mLoadMutex);
  if (mActiveProcessingState != nullptr) {
    mActiveProcessingState->model->Reset(sampleRate, preparedBlockSize);
  }
  if (mStagedProcessingState != nullptr) {
    mStagedProcessingState->model->Reset(sampleRate, preparedBlockSize);
  }
}

void NamParametricPluginAudioProcessor::releaseResources() {}

bool NamParametricPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() &&
         layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void NamParametricPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  juce::ignoreUnused(midiMessages);

  if (buffer.getNumChannels() == 0 || buffer.getNumSamples() <= 0 || mInputScratch.empty()) {
    return;
  }

  const int numSamples = buffer.getNumSamples();

  TryPromoteStagedModel();
  ApplyPendingRuntimeParameterChanges();

  const float inputGainDb = mInputGainParam != nullptr ? mInputGainParam->load() : 0.0f;
  const float outputGainDb = mOutputGainParam != nullptr ? mOutputGainParam->load() : 0.0f;
  const float inputGain = juce::Decibels::decibelsToGain(inputGainDb);
  const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);

  const float* input = buffer.getReadPointer(0);
  float* output = buffer.getWritePointer(0);
  const int chunkCapacity = static_cast<int>(mInputScratch.size());

  for (int offset = 0; offset < numSamples; offset += chunkCapacity) {
    const int chunkSize = std::min(chunkCapacity, numSamples - offset);
    for (int i = 0; i < chunkSize; ++i) {
      mInputScratch[static_cast<size_t>(i)] = input[offset + i] * inputGain;
    }

    if (mActiveProcessingState != nullptr && mActiveProcessingState->model->IsLoaded()) {
      mActiveProcessingState->model->Process(mInputScratch.data(), mOutputScratch.data(),
                                             chunkSize);
    } else {
      std::copy_n(mInputScratch.data(), chunkSize, mOutputScratch.data());
    }

    for (int i = 0; i < chunkSize; ++i) {
      output[offset + i] = mOutputScratch[static_cast<size_t>(i)] * outputGain;
    }
  }
}

juce::AudioProcessorEditor* NamParametricPluginAudioProcessor::createEditor() {
  return new NamParametricPluginAudioProcessorEditor(*this);
}

juce::AudioProcessorValueTreeState::ParameterLayout
NamParametricPluginAudioProcessor::CreateParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  const juce::NormalisableRange<float> gainRange(-12.0f, 12.0f, 0.01f);
  const auto gainAttrs = juce::AudioParameterFloatAttributes().withLabel("dB");

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(ParamIDs::inputGainDb, 1), "Input Gain", gainRange, 0.0f, gainAttrs));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(ParamIDs::outputGainDb, 1), "Output Gain", gainRange, 0.0f, gainAttrs));

  return {params.begin(), params.end()};
}

void NamParametricPluginAudioProcessor::TryPromoteStagedModel() {
  std::unique_lock<std::mutex> lock(mLoadMutex, std::try_to_lock);
  if (!lock.owns_lock() || mRetiredProcessingState != nullptr) {
    return;
  }

  if (mClearModelStaged) {
    mRetiredProcessingState = std::move(mActiveProcessingState);
    mClearModelStaged = false;
    return;
  }
  if (mStagedProcessingState == nullptr) {
    return;
  }

  mRetiredProcessingState = std::move(mActiveProcessingState);
  mActiveProcessingState = std::move(mStagedProcessingState);
}

void NamParametricPluginAudioProcessor::LoadModelAsync(const juce::File& modelFile) {
  CollectCompletedModelLoad();
  StartModelLoad(modelFile);
}

void NamParametricPluginAudioProcessor::StartModelLoad(
    const juce::File& modelFile, std::vector<RestoredRuntimeParameterValue> restoredValues) {
  if (!modelFile.existsAsFile()) {
    std::lock_guard<std::mutex> lock(mLoadMutex);
    mStatusText = "Model file does not exist: " + modelFile.getFullPathName();
    return;
  }

  std::lock_guard<std::mutex> lock(mLoadMutex);
  LoadRequest request{modelFile, std::move(restoredValues), ++mLatestLoadRequestId};
  if (mLoadFuture.has_value()) {
    mQueuedLoadRequest = std::move(request);
    mStatusText = "Queued model: " + modelFile.getFileName();
    return;
  }

  BeginModelLoadLocked(std::move(request));
}

void NamParametricPluginAudioProcessor::BeginModelLoadLocked(LoadRequest request) {
  const juce::String fullPath = request.modelFile.getFullPathName();
  const std::string modelPath = fullPath.toStdString();
  const juce::String fileName = request.modelFile.getFileName();
  const auto hostConfig = mHostProcessingConfig;

  mStatusText = "Loading model: " + fileName;
  mLoadFuture.emplace(std::async(std::launch::async, [modelPath, fullPath, hostConfig,
                                                      loadRequest = std::move(request)]() mutable {
    AsyncLoadResult result;
    result.requestId = loadRequest.requestId;
    result.restoredValues = std::move(loadRequest.restoredValues);
    auto model = std::make_unique<namparametric::dsp::NamModelEngine>();

    std::string error;
    if (!model->LoadModel(modelPath, error)) {
      result.message = "Failed to load model: " + juce::String(error);
      return result;
    }

    for (;;) {
      const uint64_t generationBefore = hostConfig->generation.load(std::memory_order_acquire);
      if ((generationBefore & 1ULL) != 0) {
        continue;
      }
      const double sampleRate = hostConfig->sampleRate.load(std::memory_order_relaxed);
      const int blockSize = std::max(hostConfig->blockSize.load(std::memory_order_relaxed),
                                     kMinimumModelResetBlockSize);
      if (generationBefore != hostConfig->generation.load(std::memory_order_acquire)) {
        continue;
      }
      model->Reset(sampleRate, blockSize);
      if (generationBefore == hostConfig->generation.load(std::memory_order_acquire)) {
        break;
      }
    }

    auto processingState = std::make_unique<ProcessingState>();
    processingState->pendingValues = model->GetParameterValues();
    processingState->mailbox =
        std::make_shared<RuntimeParameterMailbox>(processingState->pendingValues);
    processingState->model = std::move(model);

    result.success = true;
    result.loadedPath = fullPath;
    result.runtimeParameters =
        ConvertRuntimeParameters(processingState->model->GetParameterInfos());
    result.message = "Loaded model: " + juce::File(fullPath).getFileName();
    result.processingState = std::move(processingState);
    return result;
  }));
}

void NamParametricPluginAudioProcessor::CollectCompletedModelLoad() {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  mRetiredProcessingState.reset();

  if (!mLoadFuture.has_value() ||
      mLoadFuture->wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
    return;
  }

  AsyncLoadResult result = mLoadFuture->get();
  mLoadFuture.reset();
  if (result.requestId == mLatestLoadRequestId) {
    if (!result.success) {
      mStatusText = result.message;
    } else {
      ApplyRestoredValues(result);
      mStagedProcessingState = std::move(result.processingState);
      mRuntimeParameterMailbox = mStagedProcessingState->mailbox;
      mRuntimeParameters = std::move(result.runtimeParameters);
      mModelPath = result.loadedPath;
      mStatusText = result.message;
      mClearModelStaged = false;
    }
  }

  if (mQueuedLoadRequest.has_value()) {
    auto queuedRequest = std::move(*mQueuedLoadRequest);
    mQueuedLoadRequest.reset();
    BeginModelLoadLocked(std::move(queuedRequest));
  }
}

void NamParametricPluginAudioProcessor::ApplyRestoredValues(AsyncLoadResult& result) {
  if (result.processingState == nullptr || result.processingState->mailbox == nullptr ||
      result.restoredValues.empty()) {
    return;
  }

  auto& values = result.processingState->pendingValues;
  auto& params = result.runtimeParameters;
  std::vector<bool> matched(params.size(), false);
  std::vector<bool> used(result.restoredValues.size(), false);

  const auto apply = [&](const size_t paramIndex, const size_t restoredIndex) {
    const auto& param = params[paramIndex];
    float value = result.restoredValues[restoredIndex].value;
    if (param.IsSwitch()) {
      const float maximum = static_cast<float>(param.enumNames.size() - 1);
      value = std::clamp(std::round(value), 0.0f, maximum);
    } else {
      value =
          std::clamp(value, static_cast<float>(param.minValue), static_cast<float>(param.maxValue));
    }
    values[paramIndex] = value;
    matched[paramIndex] = true;
    used[restoredIndex] = true;
  };

  for (size_t restoredIndex = 0; restoredIndex < result.restoredValues.size(); ++restoredIndex) {
    const auto& restored = result.restoredValues[restoredIndex];
    if (restored.index < 0 || static_cast<size_t>(restored.index) >= params.size()) {
      continue;
    }
    const size_t paramIndex = static_cast<size_t>(restored.index);
    if (params[paramIndex].name.toStdString() == restored.name) {
      apply(paramIndex, restoredIndex);
    }
  }

  for (size_t restoredIndex = 0; restoredIndex < result.restoredValues.size(); ++restoredIndex) {
    if (used[restoredIndex]) {
      continue;
    }
    const auto& restored = result.restoredValues[restoredIndex];
    size_t matchingIndex = params.size();
    int matches = 0;
    for (size_t paramIndex = 0; paramIndex < params.size(); ++paramIndex) {
      if (!matched[paramIndex] && params[paramIndex].name.toStdString() == restored.name) {
        matchingIndex = paramIndex;
        ++matches;
      }
    }
    if (matches == 1) {
      apply(matchingIndex, restoredIndex);
    }
  }

  result.processingState->mailbox->PublishAll(values);
}

void NamParametricPluginAudioProcessor::StageModelClear() {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  ++mLatestLoadRequestId;
  mQueuedLoadRequest.reset();
  mStagedProcessingState.reset();
  mRuntimeParameterMailbox.reset();
  mRuntimeParameters.clear();
  mModelPath.clear();
  mStatusText = "No model loaded";
  mClearModelStaged = true;
}

juce::String NamParametricPluginAudioProcessor::GetStatusText() const {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  return mStatusText;
}

juce::String NamParametricPluginAudioProcessor::GetModelPath() const {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  return mModelPath;
}

bool NamParametricPluginAudioProcessor::HasModelLoaded() const {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  return !mClearModelStaged &&
         (mActiveProcessingState != nullptr || mStagedProcessingState != nullptr);
}

std::vector<NamParametricPluginAudioProcessor::RuntimeParameterInfo>
NamParametricPluginAudioProcessor::GetRuntimeParameters() const {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  return mRuntimeParameters;
}

void NamParametricPluginAudioProcessor::SetRuntimeParameterValue(const size_t index,
                                                                 const double value) {
  std::shared_ptr<RuntimeParameterMailbox> mailbox;
  float sanitizedValue = 0.0f;
  {
    std::lock_guard<std::mutex> lock(mLoadMutex);
    if (index >= mRuntimeParameters.size() || !std::isfinite(value)) {
      return;
    }

    const auto& param = mRuntimeParameters[index];
    if (param.IsSwitch()) {
      sanitizedValue = static_cast<float>(
          std::clamp(std::round(value), 0.0, static_cast<double>(param.enumNames.size() - 1)));
    } else {
      sanitizedValue = static_cast<float>(std::clamp(value, param.minValue, param.maxValue));
    }
    mailbox = mRuntimeParameterMailbox;
  }

  if (mailbox != nullptr) {
    mailbox->Publish(index, sanitizedValue);
  }
}

std::optional<double> NamParametricPluginAudioProcessor::GetRuntimeParameterValue(
    const size_t index) const {
  std::shared_ptr<RuntimeParameterMailbox> mailbox;
  {
    std::lock_guard<std::mutex> lock(mLoadMutex);
    mailbox = mRuntimeParameterMailbox;
  }
  if (mailbox == nullptr) {
    return std::nullopt;
  }
  const auto value = mailbox->Read(index);
  return value.has_value() ? std::optional<double>(*value) : std::nullopt;
}

void NamParametricPluginAudioProcessor::ApplyPendingRuntimeParameterChanges() {
  if (mActiveProcessingState == nullptr || mActiveProcessingState->mailbox == nullptr ||
      mActiveProcessingState->pendingValues.empty()) {
    return;
  }

  uint64_t version = mActiveProcessingState->consumedVersion;
  if (!mActiveProcessingState->mailbox->TryReadStable(mActiveProcessingState->pendingValues,
                                                      version) ||
      version == mActiveProcessingState->consumedVersion) {
    return;
  }

  if (mActiveProcessingState->model->SetParameterValues(mActiveProcessingState->pendingValues)) {
    mActiveProcessingState->consumedVersion = version;
  }
}

void NamParametricPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  CollectCompletedModelLoad();
  auto state = mValueTree.copyState();
  juce::String modelPath;
  std::vector<RuntimeParameterInfo> params;
  std::shared_ptr<RuntimeParameterMailbox> mailbox;

  {
    std::lock_guard<std::mutex> lock(mLoadMutex);
    modelPath = mModelPath;
    params = mRuntimeParameters;
    mailbox = mRuntimeParameterMailbox;
  }
  state.setProperty(kModelPathStateKey, modelPath, nullptr);

  auto child = state.getChildWithName(kRuntimeParamsStateType);
  while (child.isValid()) {
    state.removeChild(child, nullptr);
    child = state.getChildWithName(kRuntimeParamsStateType);
  }

  if (mailbox != nullptr && mailbox->Size() == params.size()) {
    std::vector<float> values(params.size());
    uint64_t version = 0;
    if (mailbox->TryReadStable(values, version)) {
      state.addChild(CreateRuntimeParameterState(params, values), -1, nullptr);
    }
  }

  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void NamParametricPluginAudioProcessor::setStateInformation(const void* data,
                                                            const int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml == nullptr) {
    return;
  }

  auto tree = juce::ValueTree::fromXml(*xml);
  if (!tree.isValid() || !tree.hasType(mValueTree.state.getType())) {
    return;
  }

  mValueTree.replaceState(tree);
  std::vector<RestoredRuntimeParameterValue> restoredValues;
  const auto runtimeTree = tree.getChildWithName(kRuntimeParamsStateType);
  if (runtimeTree.isValid()) {
    restoredValues.reserve(static_cast<size_t>(runtimeTree.getNumChildren()));
    for (int i = 0; i < runtimeTree.getNumChildren(); ++i) {
      const auto paramTree = runtimeTree.getChild(i);
      if (!paramTree.hasType(kRuntimeParamStateType)) {
        continue;
      }
      RestoredRuntimeParameterValue restored;
      restored.index = static_cast<int>(paramTree.getProperty(kRuntimeParamIndexStateKey, -1));
      restored.name = paramTree.getProperty(kRuntimeParamNameStateKey).toString().toStdString();
      restored.value = static_cast<float>(
          static_cast<double>(paramTree.getProperty(kRuntimeParamValueStateKey)));
      if (!restored.name.empty() && std::isfinite(restored.value)) {
        restoredValues.push_back(std::move(restored));
      }
    }
  }

  const juce::String restoredPath = tree.getProperty(kModelPathStateKey).toString();
  if (restoredPath.isEmpty()) {
    StageModelClear();
    return;
  }

  const juce::File modelFile(restoredPath);
  if (modelFile.existsAsFile() && modelFile.hasReadAccess()) {
    StartModelLoad(modelFile, std::move(restoredValues));
  } else {
    std::lock_guard<std::mutex> lock(mLoadMutex);
    mStatusText = "Stored model path is unavailable: " + restoredPath;
  }
}

void NamParametricPluginAudioProcessor::timerCallback() { CollectCompletedModelLoad(); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new NamParametricPluginAudioProcessor();
}
