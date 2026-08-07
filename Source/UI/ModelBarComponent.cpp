#include "ModelBarComponent.h"

#include "NamColours.h"

namespace {
constexpr int kSelectButtonWidth = 150;
constexpr int kDotDiameter = 8;
constexpr int kClearButtonWidth = 22;
}  // namespace

ModelBarComponent::ModelBarComponent() {
  mSelectButton.setButtonText("Select Model");
  addAndMakeVisible(mSelectButton);

  mNameLabel.setJustificationType(juce::Justification::centredLeft);
  mNameLabel.setFont(juce::Font(juce::FontOptions(15.0f)));
  mNameLabel.setColour(juce::Label::textColourId, nam::ui::Colours::textSecondary);
  addAndMakeVisible(mNameLabel);

  mClearButton.setButtonText(juce::CharPointer_UTF8("\xc3\x97"));
  mClearButton.setColour(juce::TextButton::buttonColourId, nam::ui::Colours::track);
  mClearButton.setColour(juce::TextButton::textColourOffId, nam::ui::Colours::textSecondary);
  addAndMakeVisible(mClearButton);

  SetModelInfo(false, "No model loaded");
}

void ModelBarComponent::SetModelInfo(const bool isLoaded, const juce::String& text) {
  mIsLoaded = isLoaded;
  mNameLabel.setText(text, juce::dontSendNotification);
  mNameLabel.setColour(juce::Label::textColourId,
                       isLoaded ? nam::ui::Colours::textPrimary : nam::ui::Colours::textTertiary);
  mClearButton.setVisible(isLoaded);
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

  mClearButton.setBounds(bounds.removeFromRight(kClearButtonWidth).withSizeKeepingCentre(
      kClearButtonWidth, kClearButtonWidth));
  bounds.removeFromRight(6);

  mDotBounds = bounds.removeFromLeft(kDotDiameter).withSizeKeepingCentre(kDotDiameter, kDotDiameter);
  bounds.removeFromLeft(8);
  mNameLabel.setBounds(bounds);
}
