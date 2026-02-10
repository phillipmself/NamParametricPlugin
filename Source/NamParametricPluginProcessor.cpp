#include "NamParametricPluginProcessor.h"

#include <algorithm>
#include <chrono>

#include "NamParametricPluginEditor.h"

namespace {
constexpr const char* kModelPathStateKey = "modelPath";
constexpr const char* kRuntimeParamsStateType = "RUNTIME_PARAMS";
constexpr const char* kRuntimeParamStateType = "PARAM";
constexpr const char* kRuntimeParamNameStateKey = "name";
constexpr const char* kRuntimeParamValueStateKey = "value";

std::vector<NamParametricPluginAudioProcessor::RuntimeParameterInfo> ConvertRuntimeParameters(
    const std::vector<namparametric::dsp::ModelParameterInfo>& params) {
  std::vector<NamParametricPluginAudioProcessor::RuntimeParameterInfo> out;
  out.reserve(params.size());

  for (const auto& param : params) {
    NamParametricPluginAudioProcessor::RuntimeParameterInfo info;
    info.name = param.name;
    info.isBoolean = param.isBoolean;
    info.defaultValue = param.defaultValue;
    info.minValue = param.minValue;
    info.maxValue = param.maxValue;
    out.push_back(std::move(info));
  }

  return out;
}

juce::ValueTree CreateRuntimeParameterState(
    const std::unordered_map<std::string, double>& runtimeValues) {
  juce::ValueTree runtimeTree(kRuntimeParamsStateType);
  for (const auto& [name, value] : runtimeValues) {
    juce::ValueTree paramTree(kRuntimeParamStateType);
    paramTree.setProperty(kRuntimeParamNameStateKey, juce::String(name), nullptr);
    paramTree.setProperty(kRuntimeParamValueStateKey, value, nullptr);
    runtimeTree.addChild(paramTree, -1, nullptr);
  }
  return runtimeTree;
}

std::unordered_map<std::string, double> ParseRuntimeParameterState(const juce::ValueTree& state) {
  std::unordered_map<std::string, double> runtimeValues;

  const auto runtimeTree = state.getChildWithName(kRuntimeParamsStateType);
  if (!runtimeTree.isValid()) {
    return runtimeValues;
  }

  for (int i = 0; i < runtimeTree.getNumChildren(); ++i) {
    const auto paramTree = runtimeTree.getChild(i);
    if (!paramTree.hasType(kRuntimeParamStateType)) {
      continue;
    }

    const juce::String name = paramTree.getProperty(kRuntimeParamNameStateKey).toString();
    if (name.isEmpty()) {
      continue;
    }

    const auto valueVar = paramTree.getProperty(kRuntimeParamValueStateKey);
    const double value = static_cast<double>(valueVar);
    runtimeValues[name.toStdString()] = value;
  }

  return runtimeValues;
}
}  // namespace

NamParametricPluginAudioProcessor::NamParametricPluginAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::mono(), true)
                               .withOutput("Output", juce::AudioChannelSet::mono(), true)),
      mValueTree(*this, nullptr, "PARAMS", CreateParameterLayout()) {}

void NamParametricPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  mCurrentSampleRate.store(sampleRate, std::memory_order_release);
  mCurrentBlockSize.store(samplesPerBlock, std::memory_order_release);

  mInputScratch.assign(static_cast<size_t>(samplesPerBlock), 0.0f);
  mOutputScratch.assign(static_cast<size_t>(samplesPerBlock), 0.0f);

  if (mModel != nullptr) {
    mModel->Reset(sampleRate, samplesPerBlock);
  }
}

void NamParametricPluginAudioProcessor::releaseResources() {}

bool NamParametricPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() &&
         layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void NamParametricPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  TryApplyStagedModel();
  ApplyPendingRuntimeParameterChanges();

  if (buffer.getNumChannels() == 0) {
    return;
  }

  const int numSamples = buffer.getNumSamples();
  if (numSamples <= 0) {
    return;
  }

  if (mInputScratch.size() < static_cast<size_t>(numSamples)) {
    mInputScratch.resize(static_cast<size_t>(numSamples), 0.0f);
    mOutputScratch.resize(static_cast<size_t>(numSamples), 0.0f);
  }

  const float inputGainDb = mValueTree.getRawParameterValue(ParamIDs::inputGainDb)->load();
  const float outputGainDb = mValueTree.getRawParameterValue(ParamIDs::outputGainDb)->load();
  const float inputGain = juce::Decibels::decibelsToGain(inputGainDb);
  const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);

  const float* in = buffer.getReadPointer(0);
  for (int i = 0; i < numSamples; ++i) {
    mInputScratch[static_cast<size_t>(i)] = in[i] * inputGain;
  }

  if (mModel != nullptr && mModel->IsLoaded()) {
    mModel->Process(mInputScratch.data(), mOutputScratch.data(), numSamples);
  } else {
    std::copy_n(mInputScratch.data(), numSamples, mOutputScratch.data());
  }

  float* out = buffer.getWritePointer(0);
  for (int i = 0; i < numSamples; ++i) {
    out[i] = mOutputScratch[static_cast<size_t>(i)] * outputGain;
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

void NamParametricPluginAudioProcessor::TryApplyStagedModel() {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  if (!mLoadFuture.has_value()) {
    return;
  }

  auto& future = *mLoadFuture;
  if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
    return;
  }

  AsyncLoadResult result = future.get();
  mLoadFuture.reset();

  if (result.success) {
    mModel = std::move(result.model);
    mModelPath = result.loadedPath;
    mRuntimeParameters = std::move(result.runtimeParameters);
    {
      std::lock_guard<std::mutex> runtimeLock(mRuntimeParameterMutex);
      mRuntimeParameterValues = std::move(result.runtimeParameterValues);
      mPendingRuntimeParameterValues.clear();
      if (mHasPendingRestoredRuntimeValues) {
        for (const auto& [name, value] : mRestoredRuntimeParameterValues) {
          const auto existing = mRuntimeParameterValues.find(name);
          if (existing == mRuntimeParameterValues.end()) {
            continue;
          }

          existing->second = value;
          mPendingRuntimeParameterValues[name] = value;
        }

        mRestoredRuntimeParameterValues.clear();
        mHasPendingRestoredRuntimeValues = false;
      }
    }
    mStatusText = result.message;
  } else {
    mStatusText = result.message;
  }
}

void NamParametricPluginAudioProcessor::LoadModelAsync(const juce::File& modelFile) {
  if (!modelFile.existsAsFile()) {
    std::lock_guard<std::mutex> lock(mLoadMutex);
    mStatusText = "Model file does not exist: " + modelFile.getFullPathName();
    return;
  }

  const juce::String fullPath = modelFile.getFullPathName();
  const std::string modelPath = fullPath.toStdString();
  const double sampleRate = mCurrentSampleRate.load(std::memory_order_acquire);
  const int blockSize = mCurrentBlockSize.load(std::memory_order_acquire);

  std::lock_guard<std::mutex> lock(mLoadMutex);
  mStatusText = "Loading model: " + modelFile.getFileName();

  mLoadFuture.emplace(
      std::async(std::launch::async, [modelPath, fullPath, sampleRate, blockSize]() {
        AsyncLoadResult result;
        auto stagedModel = std::make_unique<namparametric::dsp::NamModelEngine>();

        std::string error;
        if (!stagedModel->LoadModel(modelPath, error)) {
          result.success = false;
          result.message = "Failed to load model: " + juce::String(error);
          return result;
        }

        stagedModel->Reset(sampleRate, blockSize);

        result.success = true;
        result.loadedPath = fullPath;
        result.runtimeParameters = ConvertRuntimeParameters(stagedModel->GetParameterInfos());
        result.runtimeParameterValues = stagedModel->GetParameterValuesByName();
        result.message = "Loaded model: " + juce::File(fullPath).getFileName();
        result.model = std::move(stagedModel);
        return result;
      }));
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
  return mModel != nullptr && mModel->IsLoaded();
}

std::vector<NamParametricPluginAudioProcessor::RuntimeParameterInfo>
NamParametricPluginAudioProcessor::GetRuntimeParameters() const {
  std::lock_guard<std::mutex> lock(mLoadMutex);
  return mRuntimeParameters;
}

void NamParametricPluginAudioProcessor::SetRuntimeParameterValue(const juce::String& name,
                                                                 double value) {
  if (name.isEmpty()) {
    return;
  }

  const std::string key = name.toStdString();

  std::lock_guard<std::mutex> lock(mRuntimeParameterMutex);
  mRuntimeParameterValues[key] = value;
  mPendingRuntimeParameterValues[key] = value;
}

std::optional<double> NamParametricPluginAudioProcessor::GetRuntimeParameterValue(
    const juce::String& name) const {
  if (name.isEmpty()) {
    return std::nullopt;
  }

  const std::string key = name.toStdString();
  std::lock_guard<std::mutex> lock(mRuntimeParameterMutex);
  const auto it = mRuntimeParameterValues.find(key);
  if (it == mRuntimeParameterValues.end()) {
    return std::nullopt;
  }

  return it->second;
}

void NamParametricPluginAudioProcessor::ApplyPendingRuntimeParameterChanges() {
  if (mModel == nullptr || !mModel->IsLoaded()) {
    return;
  }

  std::unordered_map<std::string, double> pending;
  {
    std::lock_guard<std::mutex> lock(mRuntimeParameterMutex);
    if (mPendingRuntimeParameterValues.empty()) {
      return;
    }

    pending.swap(mPendingRuntimeParameterValues);
  }

  for (const auto& [name, value] : pending) {
    std::string ignoredError;
    mModel->SetParameterValue(name, value, ignoredError);
  }
}

void NamParametricPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto state = mValueTree.copyState();
  std::unordered_map<std::string, double> runtimeValues;

  {
    std::lock_guard<std::mutex> lock(mLoadMutex);
    state.setProperty(kModelPathStateKey, mModelPath, nullptr);
  }

  {
    std::lock_guard<std::mutex> lock(mRuntimeParameterMutex);
    runtimeValues = mRuntimeParameterValues;
  }

  auto child = state.getChildWithName(kRuntimeParamsStateType);
  while (child.isValid()) {
    state.removeChild(child, nullptr);
    child = state.getChildWithName(kRuntimeParamsStateType);
  }

  if (!runtimeValues.empty()) {
    state.addChild(CreateRuntimeParameterState(runtimeValues), -1, nullptr);
  }

  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void NamParametricPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml == nullptr) {
    return;
  }

  auto tree = juce::ValueTree::fromXml(*xml);
  if (tree.isValid() && tree.hasType(mValueTree.state.getType())) {
    mValueTree.replaceState(tree);
    const auto restoredRuntimeValues = ParseRuntimeParameterState(tree);

    const juce::String restoredPath = tree.getProperty(kModelPathStateKey).toString();
    bool shouldReloadModel = false;

    if (restoredPath.isNotEmpty()) {
      {
        std::lock_guard<std::mutex> lock(mLoadMutex);
        mModelPath = restoredPath;
      }

      const juce::File modelFile(restoredPath);
      if (modelFile.existsAsFile() && modelFile.hasReadAccess()) {
        shouldReloadModel = true;
        LoadModelAsync(modelFile);
      } else {
        std::lock_guard<std::mutex> lock(mLoadMutex);
        mStatusText = "Stored model path is unavailable: " + restoredPath;
      }
    }

    {
      std::lock_guard<std::mutex> lock(mRuntimeParameterMutex);
      mRuntimeParameterValues = restoredRuntimeValues;
      mPendingRuntimeParameterValues.clear();
      mRestoredRuntimeParameterValues = restoredRuntimeValues;
      mHasPendingRestoredRuntimeValues = !mRestoredRuntimeParameterValues.empty();

      if (!shouldReloadModel && !mRestoredRuntimeParameterValues.empty()) {
        mPendingRuntimeParameterValues = mRestoredRuntimeParameterValues;
        mRestoredRuntimeParameterValues.clear();
        mHasPendingRestoredRuntimeValues = false;
      }
    }
  }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new NamParametricPluginAudioProcessor();
}
