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

  mLoadModelButton.setButtonText("Load .nam Model");
  mLoadModelButton.onClick = [this]() {
    mModelChooser =
        std::make_unique<juce::FileChooser>("Select a NAM model", juce::File(), "*.nam");

    const int chooserFlags =
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    mModelChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
      const juce::File selectedFile = chooser.getResult();
      if (selectedFile.existsAsFile()) {
        mProcessor.LoadModelAsync(selectedFile);
      }
      mModelChooser.reset();
    });
  };
  addAndMakeVisible(mLoadModelButton);

  mModelPathLabel.setText("Model: (none)", juce::dontSendNotification);
  mModelPathLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(mModelPathLabel);

  mStatusLabel.setText(mProcessor.GetStatusText(), juce::dontSendNotification);
  mStatusLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(mStatusLabel);

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
      mProcessor.mValueTree, NamParametricPluginAudioProcessor::ParamIDs::outputGainDb,
      mOutputGain);

  startTimerHz(12);
  setSize(560, 260);
}

void NamParametricPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black);
  g.setColour(juce::Colours::white);
  g.setFont(17.0f);
}

void NamParametricPluginAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds().reduced(12);
  mTitle.setBounds(bounds.removeFromTop(36));

  auto modelRow = bounds.removeFromTop(34);
  mLoadModelButton.setBounds(modelRow.removeFromLeft(130));
  modelRow.removeFromLeft(8);
  mModelPathLabel.setBounds(modelRow);

  mStatusLabel.setBounds(bounds.removeFromTop(30));
  bounds.removeFromTop(8);

  auto row1 = bounds.removeFromTop(36);
  mInputLabel.setBounds(row1.removeFromLeft(110));
  mInputGain.setBounds(row1);

  auto row2 = bounds.removeFromTop(36);
  mOutputLabel.setBounds(row2.removeFromLeft(110));
  mOutputGain.setBounds(row2);
}

void NamParametricPluginAudioProcessorEditor::timerCallback() {
  const auto modelPath = mProcessor.GetModelPath();
  if (modelPath.isEmpty()) {
    mModelPathLabel.setText("Model: (none)", juce::dontSendNotification);
  } else {
    mModelPathLabel.setText("Model: " + modelPath, juce::dontSendNotification);
  }

  mStatusLabel.setText(mProcessor.GetStatusText(), juce::dontSendNotification);
}
