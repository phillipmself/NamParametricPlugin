#include "ModelBarComponent.h"

#include "NamColours.h"

namespace {
constexpr int kSelectButtonWidth = 150;
constexpr int kDotDiameter = 8;
}  // namespace

ModelBarComponent::ModelBarComponent() {
  mSelectButton.setButtonText("Select Model");
  addAndMakeVisible(mSelectButton);

  mNameLabel.setJustificationType(juce::Justification::centredLeft);
  mNameLabel.setFont(juce::Font(juce::FontOptions(15.0f)));
  mNameLabel.setColour(juce::Label::textColourId, nam::ui::Colours::textSecondary);
  addAndMakeVisible(mNameLabel);

  SetModelInfo(false, "No model loaded");
}

void ModelBarComponent::SetModelInfo(const bool isLoaded, const juce::String& text) {
  mIsLoaded = isLoaded;
  mNameLabel.setText(text, juce::dontSendNotification);
  mNameLabel.setColour(juce::Label::textColourId,
                       isLoaded ? nam::ui::Colours::textPrimary : nam::ui::Colours::textTertiary);
  repaint();
}

void ModelBarComponent::paint(juce::Graphics& g) {
  g.fillAll(nam::ui::Colours::topBarBackground);
  g.setColour(nam::ui::Colours::hairline);
  g.fillRect(getLocalBounds().removeFromTop(1));

  g.setColour(mIsLoaded ? nam::ui::Colours::accent : nam::ui::Colours::track);
  g.fillEllipse(mDotBounds.toFloat());
}

void ModelBarComponent::resized() {
  auto bounds = getLocalBounds().reduced(18, 12);
  mSelectButton.setBounds(bounds.removeFromLeft(kSelectButtonWidth));
  bounds.removeFromLeft(14);

  mDotBounds = bounds.removeFromLeft(kDotDiameter).withSizeKeepingCentre(kDotDiameter, kDotDiameter);
  bounds.removeFromLeft(8);
  mNameLabel.setBounds(bounds);
}
