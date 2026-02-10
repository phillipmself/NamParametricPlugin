#include "NamParametricPluginEditor.h"

void NamParametricPluginAudioProcessorEditor::InitializeGainSlider(juce::Slider& slider) {
  slider.setSliderStyle(juce::Slider::LinearHorizontal);
  slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
}

NamParametricPluginAudioProcessorEditor::NamParametricPluginAudioProcessorEditor(
    NamParametricPluginAudioProcessor& pluginProcessor)
    : juce::AudioProcessorEditor(&pluginProcessor), mProcessor(pluginProcessor) {
  mTitle.setText("NAM Parametric Plugin", juce::dontSendNotification);
  mTitle.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(mTitle);

  mInputLabel.setText("Input Gain", juce::dontSendNotification);
  mOutputLabel.setText("Output Gain", juce::dontSendNotification);
  addAndMakeVisible(mInputLabel);
  addAndMakeVisible(mOutputLabel);

  InitializeGainSlider(mInputGain);
  InitializeGainSlider(mOutputGain);
  addAndMakeVisible(mInputGain);
  addAndMakeVisible(mOutputGain);

  mInputAttachment = std::make_unique<SliderAttachment>(
      mProcessor.mValueTree, NamParametricPluginAudioProcessor::ParamIDs::inputGainDb, mInputGain);
  mOutputAttachment = std::make_unique<SliderAttachment>(
      mProcessor.mValueTree, NamParametricPluginAudioProcessor::ParamIDs::outputGainDb, mOutputGain);

  setSize(480, 220);
}

void NamParametricPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black);
  g.setColour(juce::Colours::white);
  g.setFont(17.0f);
}

void NamParametricPluginAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds().reduced(12);
  mTitle.setBounds(bounds.removeFromTop(40));

  auto row1 = bounds.removeFromTop(36);
  mInputLabel.setBounds(row1.removeFromLeft(110));
  mInputGain.setBounds(row1);

  auto row2 = bounds.removeFromTop(36);
  mOutputLabel.setBounds(row2.removeFromLeft(110));
  mOutputGain.setBounds(row2);
}
