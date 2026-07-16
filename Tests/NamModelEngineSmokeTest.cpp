#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "DSP/NamModelEngine.h"

namespace {
constexpr double kHostSampleRate = 44100.0;
constexpr int kBlockSize = 257;
constexpr int kRenderLength = 4096;

std::vector<float> Render(namparametric::dsp::NamModelEngine& engine) {
  std::vector<float> input(kRenderLength);
  std::vector<float> output(kRenderLength);
  for (int i = 0; i < kRenderLength; ++i) {
    const auto time = static_cast<double>(i) / kHostSampleRate;
    input[static_cast<size_t>(i)] =
        static_cast<float>(0.1 * std::sin(2.0 * std::numbers::pi * 220.0 * time) +
                           0.03 * std::sin(2.0 * std::numbers::pi * 997.0 * time));
  }

  for (int offset = 0; offset < kRenderLength; offset += kBlockSize) {
    const int count = std::min(kBlockSize, kRenderLength - offset);
    engine.Process(input.data() + offset, output.data() + offset, count);
  }
  return output;
}

double RootMeanSquare(const std::span<const float> values) {
  double sum = 0.0;
  for (const float value : values) {
    if (!std::isfinite(value)) {
      throw std::runtime_error("Rendered output contains a non-finite sample.");
    }
    sum += static_cast<double>(value) * value;
  }
  return std::sqrt(sum / static_cast<double>(values.size()));
}

double DifferenceRootMeanSquare(const std::span<const float> lhs,
                                const std::span<const float> rhs) {
  if (lhs.size() != rhs.size()) {
    throw std::runtime_error("Render lengths differ.");
  }
  double sum = 0.0;
  for (size_t i = 0; i < lhs.size(); ++i) {
    const double difference = static_cast<double>(lhs[i]) - rhs[i];
    sum += difference * difference;
  }
  return std::sqrt(sum / static_cast<double>(lhs.size()));
}

void TestModel(const std::string& label, const std::string& path, const size_t expectedParams) {
  namparametric::dsp::NamModelEngine engine;
  std::string error;
  if (!engine.LoadModel(path, error)) {
    throw std::runtime_error(label + " failed to load: " + error);
  }
  if (engine.GetParameterInfos().size() != expectedParams ||
      engine.GetParameterValues().size() != expectedParams) {
    throw std::runtime_error(label + " exposed an unexpected parameter count.");
  }

  engine.Reset(kHostSampleRate, kBlockSize);
  const auto nominalOutput = Render(engine);

  auto alternateValues = engine.GetParameterValues();
  const auto& infos = engine.GetParameterInfos();
  for (size_t i = 0; i < alternateValues.size(); ++i) {
    if (infos[i].IsSwitch()) {
      const int choices = static_cast<int>(infos[i].enumNames.size());
      alternateValues[i] =
          static_cast<float>((static_cast<int>(std::round(alternateValues[i])) + 1) % choices);
    } else {
      const float midpoint = 0.5f * (infos[i].minValue + infos[i].maxValue);
      alternateValues[i] = alternateValues[i] <= midpoint ? infos[i].maxValue : infos[i].minValue;
    }
  }

  if (!engine.SetParameterValues(alternateValues)) {
    throw std::runtime_error(label + " rejected a valid full parameter vector.");
  }
  engine.Reset(kHostSampleRate, kBlockSize);
  const auto alternateOutput = Render(engine);

  const double nominalRms = RootMeanSquare(nominalOutput);
  const double alternateRms = RootMeanSquare(alternateOutput);
  const double differenceRms = DifferenceRootMeanSquare(nominalOutput, alternateOutput);
  if (nominalRms <= 1.0e-8 || alternateRms <= 1.0e-8) {
    throw std::runtime_error(label + " produced effectively silent output.");
  }
  if (differenceRms <= 1.0e-7) {
    throw std::runtime_error(label + " output did not respond to parameter changes.");
  }

  std::cout << label << ": params=" << expectedParams << ", nominal_rms=" << nominalRms
            << ", alternate_rms=" << alternateRms << ", difference_rms=" << differenceRms << '\n';
}
}  // namespace

int main(const int argc, const char* const* argv) {
  if (argc != 3) {
    std::cerr << "Usage: nam_model_smoke_test <ConcatWaveNet.nam> <HyperWaveNet.nam>\n";
    return 2;
  }

  try {
    TestModel("ConcatWaveNet", argv[1], 5);
    TestModel("HyperWaveNet", argv[2], 2);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
