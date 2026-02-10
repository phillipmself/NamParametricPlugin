#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "DSP/NamModelEngine.h"

class NamParametricPluginAudioProcessor final : public juce::AudioProcessor {
 public:
  struct ParamIDs {
    static constexpr const char* inputGainDb = "inputGainDb";
    static constexpr const char* outputGainDb = "outputGainDb";
  };

  struct RuntimeParameterInfo {
    juce::String name;
    bool isBoolean = false;
    double defaultValue = 0.0;
    std::optional<double> minValue;
    std::optional<double> maxValue;
  };

  NamParametricPluginAudioProcessor();
  ~NamParametricPluginAudioProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "NAM Parametric Plugin"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  void LoadModelAsync(const juce::File& modelFile);
  juce::String GetStatusText() const;
  juce::String GetModelPath() const;
  bool HasModelLoaded() const;
  std::vector<RuntimeParameterInfo> GetRuntimeParameters() const;
  void SetRuntimeParameterValue(const juce::String& name, double value);
  std::optional<double> GetRuntimeParameterValue(const juce::String& name) const;

  juce::AudioProcessorValueTreeState mValueTree;

 private:
  struct AsyncLoadResult {
    bool success = false;
    std::unique_ptr<namparametric::dsp::NamModelEngine> model;
    juce::String message;
    juce::String loadedPath;
    std::vector<RuntimeParameterInfo> runtimeParameters;
    std::unordered_map<std::string, double> runtimeParameterValues;
  };

  juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout();
  void TryApplyStagedModel();
  void ApplyPendingRuntimeParameterChanges();

  mutable std::mutex mLoadMutex;
  std::optional<std::future<AsyncLoadResult>> mLoadFuture;

  std::unique_ptr<namparametric::dsp::NamModelEngine> mModel;
  juce::String mStatusText = "No model loaded";
  juce::String mModelPath;
  std::vector<RuntimeParameterInfo> mRuntimeParameters;
  mutable std::mutex mRuntimeParameterMutex;
  std::unordered_map<std::string, double> mRuntimeParameterValues;
  std::unordered_map<std::string, double> mPendingRuntimeParameterValues;
  std::unordered_map<std::string, double> mRestoredRuntimeParameterValues;
  bool mHasPendingRestoredRuntimeValues = false;

  std::vector<float> mInputScratch;
  std::vector<float> mOutputScratch;

  std::atomic<double> mCurrentSampleRate{48000.0};
  std::atomic<int> mCurrentBlockSize{512};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamParametricPluginAudioProcessor)
};
