#include "DSP/IrEngine.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace namparametric::dsp {

bool IrEngine::LoadIr(const std::string& irPath, const double sampleRate,
                      std::string& errorMessage) {
  errorMessage.clear();

  auto ir = std::make_unique<::dsp::ImpulseResponse>(irPath.c_str(), sampleRate);
  if (ir->GetWavState() != ::dsp::wav::LoadReturnCode::SUCCESS) {
    errorMessage = ::dsp::wav::GetMsgForLoadReturnCode(ir->GetWavState());
    return false;
  }

  mIr = std::move(ir);
  mSampleRate = sampleRate;
  return true;
}

void IrEngine::Reset(const double sampleRate, const int maxBlockSize) {
  if (mIr == nullptr) {
    return;
  }

  constexpr double kSampleRateEpsilon = 0.5;
  if (std::abs(sampleRate - mSampleRate) > kSampleRateEpsilon) {
    mIr = std::make_unique<::dsp::ImpulseResponse>(mIr->GetData(), sampleRate);
    mSampleRate = sampleRate;
  }

  // Warm up the internal buffer allocation now, off the audio thread.
  const size_t warmupSize = static_cast<size_t>(std::max(maxBlockSize, 1));
  std::vector<float> warmupInput(warmupSize, 0.0f);
  std::vector<float> warmupOutput(warmupSize, 0.0f);
  Process(warmupInput.data(), warmupOutput.data(), static_cast<int>(warmupSize));
}

void IrEngine::Process(const float* input, float* output, const int numSamples) {
  if (numSamples <= 0) {
    return;
  }

  if (mIr == nullptr) {
    std::copy_n(input, numSamples, output);
    return;
  }

  mInputConversionScratch.resize(static_cast<size_t>(numSamples));
  std::copy_n(input, numSamples, mInputConversionScratch.begin());

  double* inputPtrs[1] = {mInputConversionScratch.data()};
  double** processed = mIr->Process(inputPtrs, 1, static_cast<size_t>(numSamples));
  std::copy_n(processed[0], numSamples, output);
}

}  // namespace namparametric::dsp
