#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "dsp.h"
#include "parametric_control.h"

namespace namparametric::dsp {

struct ModelParameterInfo {
  std::string name;
  float minValue = 0.0f;
  float maxValue = 0.0f;
  float defaultValue = 0.0f;
  std::vector<std::string> enumNames;

  [[nodiscard]] bool IsSwitch() const { return !enumNames.empty(); }
};

class NamModelEngine {
 public:
  NamModelEngine();
  ~NamModelEngine();

  bool LoadModel(const std::string& modelPath, std::string& errorMessage);

  void Reset(double sampleRate, int maxBlockSize);
  void Process(float* input, float* output, int numSamples);

  [[nodiscard]] bool IsLoaded() const { return mModel != nullptr; }
  [[nodiscard]] double GetExpectedSampleRate() const;
  [[nodiscard]] const std::vector<ModelParameterInfo>& GetParameterInfos() const;
  [[nodiscard]] const std::vector<float>& GetParameterValues() const;
  bool SetParameterValues(std::span<const float> values) noexcept;

 private:
  struct ResamplerState;
  [[nodiscard]] double GetModelSampleRateForProcessing() const;
  [[nodiscard]] bool NeedToResample() const;

  std::unique_ptr<nam::DSP> mModel;
  nam::IParametricControl* mParametricControl = nullptr;
  std::vector<ModelParameterInfo> mParameterInfos;
  std::vector<float> mParameterValues;
  double mHostSampleRate = 48000.0;
  int mMaxExternalBlockSize = 0;
  std::unique_ptr<ResamplerState> mResamplerState;
};

}  // namespace namparametric::dsp
