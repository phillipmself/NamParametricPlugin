#pragma once

#include <memory>
#include <string>
#include <vector>

#include "dsp/ImpulseResponse.h"

namespace namparametric::dsp {

// dsp::ImpulseResponse (AudioDSPTools) always processes in double precision,
// regardless of the DSP_SAMPLE_FLOAT switch, so this wrapper converts at the
// float boundary used by the rest of the plugin's audio path.
class IrEngine {
 public:
  IrEngine() = default;

  bool LoadIr(const std::string& irPath, double sampleRate, std::string& errorMessage);

  void Reset(double sampleRate, int maxBlockSize);
  void Process(const float* input, float* output, int numSamples);

  [[nodiscard]] bool IsLoaded() const { return mIr != nullptr; }

 private:
  std::unique_ptr<::dsp::ImpulseResponse> mIr;
  double mSampleRate = 48000.0;
  std::vector<double> mInputConversionScratch;
};

}  // namespace namparametric::dsp
