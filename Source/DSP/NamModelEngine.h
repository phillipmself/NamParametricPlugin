#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "dsp.h"

namespace namparametric::dsp {

struct ModelParameterInfo {
  std::string name;
  bool isBoolean = false;
  double defaultValue = 0.0;
  std::optional<double> minValue;
  std::optional<double> maxValue;
};

class NamModelEngine {
 public:
  bool LoadModel(const std::string& modelPath, std::string& errorMessage);

  void Reset(double sampleRate, int maxBlockSize);
  void Process(float* input, float* output, int numSamples);

  [[nodiscard]] bool IsLoaded() const { return mModel != nullptr; }
  [[nodiscard]] double GetExpectedSampleRate() const;
  [[nodiscard]] const std::vector<ModelParameterInfo>& GetParameterInfos() const;

  bool SetParameterValue(const std::string& name, double value, std::string& errorMessage);
  [[nodiscard]] std::unordered_map<std::string, double> GetParameterValuesByName() const;
  void ApplyParameterValues(const std::unordered_map<std::string, double>& values);

 private:
  std::unique_ptr<nam::DSP> mModel;
  std::vector<ModelParameterInfo> mParameterInfos;
};

}  // namespace namparametric::dsp
