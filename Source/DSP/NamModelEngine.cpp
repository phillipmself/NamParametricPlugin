#include "DSP/NamModelEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <numbers>
#include <stdexcept>

namespace iplug {
// AudioDSPTools resampling code assumes this is an iPlug plugin
inline constexpr double PI = std::numbers::pi_v<double>;
}  // namespace iplug

#ifndef DEFAULT_BLOCK_SIZE
#define DEFAULT_BLOCK_SIZE 1024
#endif

#include "dsp/ResamplingContainer/ResamplingContainer.h"
#include "get_dsp.h"

namespace namparametric::dsp {

namespace {
constexpr double kAssumedUnknownModelSampleRate = 48000.0;
}  // namespace

struct NamModelEngine::ResamplerState {
  explicit ResamplerState(double renderingSampleRate) : resampler(renderingSampleRate) {}

  std::function<void(NAM_SAMPLE**, NAM_SAMPLE**, int)> blockProcessFunc;
  ::dsp::ResamplingContainer<NAM_SAMPLE, 1, 12> resampler;
};

NamModelEngine::NamModelEngine() = default;
NamModelEngine::~NamModelEngine() = default;

bool NamModelEngine::LoadModel(const std::string& modelPath, std::string& errorMessage) {
  errorMessage.clear();

  try {
    auto loadedModel = nam::get_dsp(std::filesystem::path(modelPath));
    if (loadedModel == nullptr) {
      errorMessage = "Model loader returned null model.";
      return false;
    }

    std::vector<ModelParameterInfo> loadedParams;
    loadedParams.reserve(loadedModel->GetParameterCount());

    for (const auto& descriptor : loadedModel->GetParameterDescriptors()) {
      ModelParameterInfo info;
      info.name = descriptor.name;
      info.isBoolean = descriptor.type == nam::ParameterType::Boolean;
      info.defaultValue = descriptor.default_value;
      info.minValue = descriptor.min_value;
      info.maxValue = descriptor.max_value;
      loadedParams.push_back(info);
    }

    mModel = std::move(loadedModel);
    mParameterInfos = std::move(loadedParams);
    mResamplerState.reset();
    return true;
  } catch (const std::exception& ex) {
    errorMessage = ex.what();
    return false;
  }
}

void NamModelEngine::Reset(double sampleRate, int maxBlockSize) {
  if (!IsLoaded()) {
    return;
  }

  mHostSampleRate = sampleRate;
  mMaxExternalBlockSize = maxBlockSize;

  if (!NeedToResample()) {
    mResamplerState.reset();
    mModel->Reset(sampleRate, maxBlockSize);
    return;
  }

  const double modelSampleRate = GetModelSampleRateForProcessing();
  mResamplerState = std::make_unique<ResamplerState>(modelSampleRate);
  mResamplerState->blockProcessFunc = [this](NAM_SAMPLE** input, NAM_SAMPLE** output,
                                             int numFrames) {
    mModel->process(input, output, numFrames);
  };
  mResamplerState->resampler.Reset(sampleRate, maxBlockSize);

  // Match NeuralAmpModelerPlugin block-size shaping for the encapsulated model.
  const double upRatio = sampleRate / modelSampleRate;
  const int maxEncapsulatedBlockSize =
      std::max(1, static_cast<int>(std::ceil(static_cast<double>(maxBlockSize) / upRatio)));
  mModel->ResetAndPrewarm(sampleRate, maxEncapsulatedBlockSize);
}

void NamModelEngine::Process(float* input, float* output, int numSamples) {
  if (!IsLoaded()) {
    std::memcpy(output, input, sizeof(float) * static_cast<size_t>(numSamples));
    return;
  }

  NAM_SAMPLE* inputPtrs[1] = {input};
  NAM_SAMPLE* outputPtrs[1] = {output};

  if (!NeedToResample() || mResamplerState == nullptr) {
    mModel->process(inputPtrs, outputPtrs, numSamples);
    return;
  }

  if (numSamples > mMaxExternalBlockSize) {
    // Defensive fallback if host violates the configured max block size.
    mModel->process(inputPtrs, outputPtrs, numSamples);
    return;
  }

  mResamplerState->resampler.ProcessBlock(inputPtrs, outputPtrs, numSamples,
                                          mResamplerState->blockProcessFunc);
}

double NamModelEngine::GetExpectedSampleRate() const {
  if (!IsLoaded()) {
    return -1.0;
  }

  return GetModelSampleRateForProcessing();
}

const std::vector<ModelParameterInfo>& NamModelEngine::GetParameterInfos() const {
  return mParameterInfos;
}

bool NamModelEngine::SetParameterValue(const std::string& name, double value,
                                       std::string& errorMessage) {
  if (!IsLoaded()) {
    errorMessage = "No model loaded.";
    return false;
  }

  try {
    mModel->SetParameter(name, value);
    return true;
  } catch (const std::exception& ex) {
    errorMessage = ex.what();
    return false;
  }
}

std::unordered_map<std::string, double> NamModelEngine::GetParameterValuesByName() const {
  std::unordered_map<std::string, double> values;
  if (!IsLoaded()) {
    return values;
  }

  values.reserve(mParameterInfos.size());
  for (const auto& param : mParameterInfos) {
    values[param.name] = mModel->GetParameter(param.name);
  }

  return values;
}

void NamModelEngine::ApplyParameterValues(const std::unordered_map<std::string, double>& values) {
  if (!IsLoaded()) {
    return;
  }

  for (const auto& [name, value] : values) {
    std::string ignoredError;
    SetParameterValue(name, value, ignoredError);
  }
}

double NamModelEngine::GetModelSampleRateForProcessing() const {
  if (!IsLoaded()) {
    return -1.0;
  }

  const double reportedRate = mModel->GetExpectedSampleRate();
  return reportedRate > 0.0 ? reportedRate : kAssumedUnknownModelSampleRate;
}

bool NamModelEngine::NeedToResample() const {
  if (!IsLoaded()) {
    return false;
  }

  constexpr double epsilon = 0.5;
  return std::abs(mHostSampleRate - GetModelSampleRateForProcessing()) > epsilon;
}

}  // namespace namparametric::dsp
