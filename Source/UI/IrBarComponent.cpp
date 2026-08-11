#include "IrBarComponent.h"

#include "NamColours.h"

namespace {
constexpr int kSelectButtonWidth = 150;
constexpr int kDotDiameter = 8;
constexpr int kClearButtonWidth = 22;
constexpr int kEnabledSwitchWidth = 86;
// Reserves room in the bottom-right corner of the window for the About
// overlay trigger, which sits outside this component but shares its row.
constexpr int kAboutButtonReserve = 44;
}  // namespace

IrBarComponent::IrBarComponent() {
  mSelectButton.setButtonText("Select IR");
  addAndMakeVisible(mSelectButton);

  mNameLabel.setJustificationType(juce::Justification::centredLeft);
  mNameLabel.setFont(juce::Font(juce::FontOptions(15.0f)));
  mNameLabel.setColour(juce::Label::textColourId, nam::ui::Colours::textSecondary);
  addAndMakeVisible(mNameLabel);

  mClearButton.setButtonText(juce::CharPointer_UTF8("\xc3\x97"));
  mClearButton.setColour(juce::TextButton::buttonColourId, nam::ui::Colours::track);
  mClearButton.setColour(juce::TextButton::textColourOffId, nam::ui::Colours::textSecondary);
  addAndMakeVisible(mClearButton);

  mEnabledSwitch.setOptions({"Off", "On"});
  mEnabledSwitch.setSelectedIndex(1, juce::dontSendNotification);
  addAndMakeVisible(mEnabledSwitch);

  SetIrInfo(false, "No IR loaded");
}

void IrBarComponent::SetIrInfo(const bool isLoaded, const juce::String& text) {
  mIsLoaded = isLoaded;
  mNameLabel.setText(text, juce::dontSendNotification);
  mNameLabel.setColour(juce::Label::textColourId,
                       isLoaded ? nam::ui::Colours::textPrimary : nam::ui::Colours::textTertiary);
  mClearButton.setVisible(isLoaded);
  mEnabledSwitch.setEnabled(isLoaded);
  repaint();
}

void IrBarComponent::paint(juce::Graphics& g) {
  g.fillAll(nam::ui::Colours::topBarBackground);
  g.setColour(nam::ui::Colours::hairline);
  g.fillRect(getLocalBounds().removeFromTop(1));

  g.setColour(mIsLoaded ? nam::ui::Colours::accent : nam::ui::Colours::track);
  g.fillEllipse(mDotBounds.toFloat());
}

void IrBarComponent::resized() {
  auto bounds = getLocalBounds().reduced(18, 12);
  bounds.removeFromRight(kAboutButtonReserve);
  mSelectButton.setBounds(bounds.removeFromLeft(kSelectButtonWidth));
  bounds.removeFromLeft(14);

  mEnabledSwitch.setBounds(bounds.removeFromRight(kEnabledSwitchWidth));
  bounds.removeFromRight(10);

  mClearButton.setBounds(bounds.removeFromRight(kClearButtonWidth)
                             .withSizeKeepingCentre(kClearButtonWidth, kClearButtonWidth));
  bounds.removeFromRight(6);

  mDotBounds =
      bounds.removeFromLeft(kDotDiameter).withSizeKeepingCentre(kDotDiameter, kDotDiameter);
  bounds.removeFromLeft(8);
  mNameLabel.setBounds(bounds);
}
