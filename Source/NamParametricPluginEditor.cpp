#include "NamParametricPluginEditor.h"

NamParametricPluginAudioProcessorEditor::NamParametricPluginAudioProcessorEditor(
    NamParametricPluginAudioProcessor& pluginProcessor)
    : juce::AudioProcessorEditor(&pluginProcessor), mProcessor(pluginProcessor) {
  juce::ignoreUnused(mProcessor);

  mTitle.setText("NAM Parametric Plugin", juce::dontSendNotification);
  mTitle.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(mTitle);

  setSize(440, 240);
}

void NamParametricPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black);
}

void NamParametricPluginAudioProcessorEditor::resized() {
  mTitle.setBounds(getLocalBounds().removeFromTop(42));
}
