#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "NamParametricPluginProcessor.h"

class NamParametricPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer {
 public:
  explicit NamParametricPluginAudioProcessorEditor(NamParametricPluginAudioProcessor&);
  ~NamParametricPluginAudioProcessorEditor() override = default;

  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

  static void InitializeGainSlider(juce::Slider& slider);
  void timerCallback() override;

  NamParametricPluginAudioProcessor& mProcessor;
  juce::Label mTitle;
  juce::TextButton mLoadModelButton;
  juce::Label mModelPathLabel;
  juce::Label mStatusLabel;

  juce::Slider mInputGain;
  juce::Slider mOutputGain;
  juce::Label mInputLabel;
  juce::Label mOutputLabel;

  std::unique_ptr<juce::FileChooser> mModelChooser;

  std::unique_ptr<SliderAttachment> mInputAttachment;
  std::unique_ptr<SliderAttachment> mOutputAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamParametricPluginAudioProcessorEditor)
};
