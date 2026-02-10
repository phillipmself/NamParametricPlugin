#include "NamParametricPluginEditor.h"

#include <algorithm>

namespace {
constexpr int kControlRowHeight = 34;
constexpr int kRuntimeLabelWidth = 180;
constexpr int kRuntimeSectionHeight = 24;
}  // namespace

void NamParametricPluginAudioProcessorEditor::InitializeGainSlider(juce::Slider& slider) {
  slider.setSliderStyle(juce::Slider::LinearHorizontal);
  slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
}

bool NamParametricPluginAudioProcessorEditor::RuntimeParameterListsEqual(
    const std::vector<RuntimeParameterInfo>& lhs, const std::vector<RuntimeParameterInfo>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs.size(); ++i) {
    const auto& a = lhs[i];
    const auto& b = rhs[i];

    if (a.name != b.name || a.isBoolean != b.isBoolean ||
        !juce::approximatelyEqual(a.defaultValue, b.defaultValue) ||
        a.minValue.has_value() != b.minValue.has_value() ||
        a.maxValue.has_value() != b.maxValue.has_value()) {
      return false;
    }

    if (a.minValue.has_value() && !juce::approximatelyEqual(*a.minValue, *b.minValue)) {
      return false;
    }

    if (a.maxValue.has_value() && !juce::approximatelyEqual(*a.maxValue, *b.maxValue)) {
      return false;
    }
  }

  return true;
}

double NamParametricPluginAudioProcessorEditor::GetInitialRuntimeValue(
    const RuntimeParameterInfo& param) const {
  const auto storedValue =
      mProcessor.GetRuntimeParameterValue(param.name).value_or(param.defaultValue);

  if (param.isBoolean) {
    return storedValue >= 0.5 ? 1.0 : 0.0;
  }

  double minValue = param.minValue.value_or(param.defaultValue - 1.0);
  double maxValue = param.maxValue.value_or(param.defaultValue + 1.0);
  if (maxValue < minValue) {
    std::swap(minValue, maxValue);
  }
  if (juce::approximatelyEqual(minValue, maxValue)) {
    maxValue = minValue + 1.0;
  }

  return juce::jlimit(minValue, maxValue, storedValue);
}

void NamParametricPluginAudioProcessorEditor::RebuildRuntimeParameterControls(
    const std::vector<RuntimeParameterInfo>& params) {
  mRuntimeRows.clear();
  mRuntimeContent.removeAllChildren();

  for (const auto& param : params) {
    RuntimeControlRow row;
    row.name = param.name;
    row.isBoolean = param.isBoolean;

    row.label = std::make_unique<juce::Label>();
    row.label->setText(param.name, juce::dontSendNotification);
    row.label->setJustificationType(juce::Justification::centredLeft);
    mRuntimeContent.addAndMakeVisible(*row.label);

    if (param.isBoolean) {
      row.toggle = std::make_unique<juce::ToggleButton>("Enabled");
      row.toggle->setToggleState(GetInitialRuntimeValue(param) >= 0.5, juce::dontSendNotification);

      auto* toggle = row.toggle.get();
      const juce::String name = param.name;
      toggle->onClick = [this, toggle, name]() {
        mProcessor.SetRuntimeParameterValue(name, toggle->getToggleState() ? 1.0 : 0.0);
      };

      mRuntimeContent.addAndMakeVisible(*row.toggle);
    } else {
      double minValue = param.minValue.value_or(param.defaultValue - 1.0);
      double maxValue = param.maxValue.value_or(param.defaultValue + 1.0);
      if (maxValue < minValue) {
        std::swap(minValue, maxValue);
      }
      if (juce::approximatelyEqual(minValue, maxValue)) {
        maxValue = minValue + 1.0;
      }

      row.slider = std::make_unique<juce::Slider>();
      row.slider->setSliderStyle(juce::Slider::LinearHorizontal);
      row.slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 85, 20);
      const double interval = std::max(0.0001, (maxValue - minValue) / 1000.0);
      row.slider->setRange(minValue, maxValue, interval);
      row.slider->setValue(GetInitialRuntimeValue(param), juce::dontSendNotification);

      auto* slider = row.slider.get();
      const juce::String name = param.name;
      slider->onValueChange = [this, slider, name]() {
        mProcessor.SetRuntimeParameterValue(name, slider->getValue());
      };

      mRuntimeContent.addAndMakeVisible(*row.slider);
    }

    mRuntimeRows.push_back(std::move(row));
  }

  mRuntimeContent.addAndMakeVisible(mRuntimeEmptyLabel);
  mLastRuntimeParameters = params;
  resized();
}

void NamParametricPluginAudioProcessorEditor::UpdateRuntimeParameterControls() {
  const auto params = mProcessor.GetRuntimeParameters();
  if (!RuntimeParameterListsEqual(mLastRuntimeParameters, params)) {
    RebuildRuntimeParameterControls(params);
  }
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

  mRuntimeSectionLabel.setText("Model Parameters", juce::dontSendNotification);
  mRuntimeSectionLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(mRuntimeSectionLabel);

  mRuntimeViewport.setViewedComponent(&mRuntimeContent, false);
  mRuntimeViewport.setScrollBarsShown(true, false);
  addAndMakeVisible(mRuntimeViewport);

  mRuntimeEmptyLabel.setText("No dynamic model parameters.", juce::dontSendNotification);
  mRuntimeEmptyLabel.setJustificationType(juce::Justification::centredLeft);
  mRuntimeContent.addAndMakeVisible(mRuntimeEmptyLabel);

  mInputAttachment = std::make_unique<SliderAttachment>(
      mProcessor.mValueTree, NamParametricPluginAudioProcessor::ParamIDs::inputGainDb, mInputGain);
  mOutputAttachment = std::make_unique<SliderAttachment>(
      mProcessor.mValueTree, NamParametricPluginAudioProcessor::ParamIDs::outputGainDb,
      mOutputGain);

  UpdateRuntimeParameterControls();
  startTimerHz(12);
  setSize(560, 460);
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

  bounds.removeFromTop(8);
  mRuntimeSectionLabel.setBounds(bounds.removeFromTop(kRuntimeSectionHeight));
  bounds.removeFromTop(4);
  mRuntimeViewport.setBounds(bounds);

  const int contentWidth =
      juce::jmax(1, mRuntimeViewport.getWidth() - mRuntimeViewport.getScrollBarThickness());
  int y = 0;

  if (mRuntimeRows.empty()) {
    mRuntimeEmptyLabel.setVisible(true);
    mRuntimeEmptyLabel.setBounds(0, 0, contentWidth, kControlRowHeight);
    y = kControlRowHeight;
  } else {
    mRuntimeEmptyLabel.setVisible(false);

    for (auto& row : mRuntimeRows) {
      auto rowBounds = juce::Rectangle<int>(0, y, contentWidth, kControlRowHeight);
      if (row.label != nullptr) {
        row.label->setBounds(rowBounds.removeFromLeft(kRuntimeLabelWidth));
      }
      rowBounds.removeFromLeft(8);

      if (row.isBoolean && row.toggle != nullptr) {
        row.toggle->setBounds(rowBounds.removeFromLeft(140));
      } else if (!row.isBoolean && row.slider != nullptr) {
        row.slider->setBounds(rowBounds);
      }

      y += kControlRowHeight;
    }
  }

  mRuntimeContent.setSize(contentWidth, juce::jmax(y, mRuntimeViewport.getHeight()));
}

void NamParametricPluginAudioProcessorEditor::timerCallback() {
  const auto modelPath = mProcessor.GetModelPath();
  if (modelPath.isEmpty()) {
    mModelPathLabel.setText("Model: (none)", juce::dontSendNotification);
  } else {
    mModelPathLabel.setText("Model: " + modelPath, juce::dontSendNotification);
  }

  mStatusLabel.setText(mProcessor.GetStatusText(), juce::dontSendNotification);
  UpdateRuntimeParameterControls();
}
