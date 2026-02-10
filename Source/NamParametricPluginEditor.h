#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "NamParametricPluginProcessor.h"

class NamParametricPluginAudioProcessorEditor final : public juce::AudioProcessorEditor {
 public:
  explicit NamParametricPluginAudioProcessorEditor(NamParametricPluginAudioProcessor&);
  ~NamParametricPluginAudioProcessorEditor() override = default;

  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  NamParametricPluginAudioProcessor& mProcessor;
  juce::Label mTitle;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamParametricPluginAudioProcessorEditor)
};
