#include "NamParametricPluginEditor.h"

#include <algorithm>
#include <cmath>

#include "UI/NamColours.h"

namespace {
constexpr int kTopBarHeight = 96;
constexpr int kModelBarHeight = 56;
}  // namespace

bool NamParametricPluginAudioProcessorEditor::RuntimeParameterListsEqual(
    const std::vector<RuntimeParameterInfo>& lhs, const std::vector<RuntimeParameterInfo>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs.size(); ++i) {
    const auto& a = lhs[i];
    const auto& b = rhs[i];

    if (a.name != b.name || a.enumNames != b.enumNames ||
        !juce::approximatelyEqual(a.defaultValue, b.defaultValue) ||
        !juce::approximatelyEqual(a.minValue, b.minValue) ||
        !juce::approximatelyEqual(a.maxValue, b.maxValue)) {
      return false;
    }
  }

  return true;
}

double NamParametricPluginAudioProcessorEditor::GetInitialRuntimeValue(
    const size_t index, const RuntimeParameterInfo& param) const {
  const auto storedValue = mProcessor.GetRuntimeParameterValue(index).value_or(param.defaultValue);

  if (param.IsSwitch()) {
    return juce::jlimit(0.0, static_cast<double>(param.enumNames.size() - 1),
                        std::round(storedValue));
  }

  double minValue = param.minValue;
  double maxValue = param.maxValue;
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
  std::vector<double> initialValues;
  initialValues.reserve(params.size());
  for (size_t index = 0; index < params.size(); ++index) {
    initialValues.push_back(GetInitialRuntimeValue(index, params[index]));
  }

  mParametersPanel.RebuildControls(params, initialValues);
  mLastRuntimeParameters = params;
}

void NamParametricPluginAudioProcessorEditor::UpdateRuntimeParameterControls() {
  const auto params = mProcessor.GetRuntimeParameters();
  if (!RuntimeParameterListsEqual(mLastRuntimeParameters, params)) {
    RebuildRuntimeParameterControls(params);
  }
  UpdateRuntimeParameterValues();
}

void NamParametricPluginAudioProcessorEditor::UpdateRuntimeParameterValues() {
  for (size_t index = 0; index < mParametersPanel.GetControlCount(); ++index) {
    const auto value = mProcessor.GetRuntimeParameterValue(index);
    if (value.has_value()) {
      mParametersPanel.SetValue(index, *value);
    }
  }
}

void NamParametricPluginAudioProcessorEditor::UpdateModelBarInfo() {
  const bool loaded = mProcessor.HasModelLoaded();
  const auto text =
      loaded ? juce::File(mProcessor.GetModelPath()).getFileName() : mProcessor.GetStatusText();
  mModelBar.SetModelInfo(loaded, text);
}

void NamParametricPluginAudioProcessorEditor::ShowModelChooser() {
  mModelChooser = std::make_unique<juce::FileChooser>("Select a NAM model", juce::File(), "*.nam");

  const int chooserFlags =
      juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

  mModelChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
    const juce::File selectedFile = chooser.getResult();
    if (selectedFile.existsAsFile()) {
      mProcessor.LoadModelAsync(selectedFile);
    }
    mModelChooser.reset();
  });
}

NamParametricPluginAudioProcessorEditor::NamParametricPluginAudioProcessorEditor(
    NamParametricPluginAudioProcessor& pluginProcessor)
    : juce::AudioProcessorEditor(&pluginProcessor), mProcessor(pluginProcessor) {
  setLookAndFeel(&mLookAndFeel);

  addAndMakeVisible(mTopBar);
  addAndMakeVisible(mParametersPanel);
  addAndMakeVisible(mModelBar);

  mModelBar.getSelectButton().onClick = [this]() { ShowModelChooser(); };

  mParametersPanel.onValueChanged = [this](size_t index, double value) {
    mProcessor.SetRuntimeParameterValue(index, value);
  };

  mInputAttachment = std::make_unique<SliderAttachment>(
      mProcessor.mValueTree, NamParametricPluginAudioProcessor::ParamIDs::inputGainDb,
      mTopBar.getInputSlider());
  mOutputAttachment = std::make_unique<SliderAttachment>(
      mProcessor.mValueTree, NamParametricPluginAudioProcessor::ParamIDs::outputGainDb,
      mTopBar.getOutputSlider());

  UpdateRuntimeParameterControls();
  UpdateModelBarInfo();
  startTimerHz(12);
  setSize(640, 460);
}

NamParametricPluginAudioProcessorEditor::~NamParametricPluginAudioProcessorEditor() {
  setLookAndFeel(nullptr);
}

void NamParametricPluginAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(nam::ui::Colours::background);
}

void NamParametricPluginAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds();
  mTopBar.setBounds(bounds.removeFromTop(kTopBarHeight));
  mModelBar.setBounds(bounds.removeFromBottom(kModelBarHeight));
  mParametersPanel.setBounds(bounds);
}

void NamParametricPluginAudioProcessorEditor::timerCallback() {
  UpdateModelBarInfo();
  UpdateRuntimeParameterControls();
}
