#include "DSP/NamModelEngine.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>

#include "get_dsp.h"

namespace namparametric::dsp {

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

  mModel->Reset(sampleRate, maxBlockSize);
}

void NamModelEngine::Process(float* input, float* output, int numSamples) {
  if (!IsLoaded()) {
    std::memcpy(output, input, sizeof(float) * static_cast<size_t>(numSamples));
    return;
  }

  NAM_SAMPLE* inputPtrs[1] = {input};
  NAM_SAMPLE* outputPtrs[1] = {output};
  mModel->process(inputPtrs, outputPtrs, numSamples);
}

double NamModelEngine::GetExpectedSampleRate() const {
  if (!IsLoaded()) {
    return -1.0;
  }

  return mModel->GetExpectedSampleRate();
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

}  // namespace namparametric::dsp
