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
    if (loadedModel->NumInputChannels() != 1 || loadedModel->NumOutputChannels() != 1) {
      errorMessage = "Model must have exactly one input and one output channel.";
      return false;
    }

    auto* loadedParametricControl = dynamic_cast<nam::IParametricControl*>(loadedModel.get());
    std::vector<ModelParameterInfo> loadedParams;
    std::vector<float> loadedValues;

    if (loadedParametricControl != nullptr) {
      const auto& specs = loadedParametricControl->GetParamSpecs();
      const auto values = loadedParametricControl->GetParams();
      if (values.size() != specs.size()) {
        throw std::runtime_error("Parametric model returned inconsistent parameter metadata.");
      }

      loadedParams.reserve(specs.size());
      loadedValues.assign(values.begin(), values.end());
      for (const auto& spec : specs) {
        ModelParameterInfo info;
        info.name = spec.name;
        info.minValue = spec.min;
        info.maxValue = spec.max;
        info.defaultValue = spec.defaultValue;
        if (spec.type == "switch") {
          info.enumNames = spec.enum_names;
        }
        loadedParams.push_back(std::move(info));
      }
    }

    mModel = std::move(loadedModel);
    mParametricControl = loadedParametricControl;
    mParameterInfos = std::move(loadedParams);
    mParameterValues = std::move(loadedValues);
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
  mModel->Reset(sampleRate, maxEncapsulatedBlockSize);
}

void NamModelEngine::Process(float* input, float* output, int numSamples) {
  if (numSamples <= 0) {
    return;
  }

  if (!IsLoaded() || mMaxExternalBlockSize <= 0 || numSamples > mMaxExternalBlockSize) {
    std::copy_n(input, numSamples, output);
    return;
  }

  NAM_SAMPLE* inputPtrs[1] = {input};
  NAM_SAMPLE* outputPtrs[1] = {output};

  if (!NeedToResample() || mResamplerState == nullptr) {
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

const std::vector<float>& NamModelEngine::GetParameterValues() const { return mParameterValues; }

bool NamModelEngine::SetParameterValues(const std::span<const float> values) noexcept {
  if (mParametricControl == nullptr || values.size() != mParameterValues.size()) {
    return false;
  }
  try {
    mParametricControl->SetParams(values);
    std::copy(values.begin(), values.end(), mParameterValues.begin());
    return true;
  } catch (...) {
    return false;
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
