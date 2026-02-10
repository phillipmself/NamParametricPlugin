#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class NamParametricPluginAudioProcessor final : public juce::AudioProcessor {
 public:
  struct ParamIDs {
    static constexpr const char* inputGainDb = "inputGainDb";
    static constexpr const char* outputGainDb = "outputGainDb";
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

  juce::AudioProcessorValueTreeState mValueTree;

 private:
  juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamParametricPluginAudioProcessor)
};
