#include "TopBarComponent.h"

#include "NamColours.h"

namespace {
constexpr int kKnobColumnWidth = 108;
constexpr int kKnobSize = 56;
constexpr int kTagHeight = 18;
constexpr int kValueHeight = 18;
constexpr int kColumnGap = 3;
constexpr int kColumnVerticalPadding = 6;
}  // namespace

void TopBarComponent::ConfigureTag(juce::Label& label, const juce::String& text) {
  label.setText(text, juce::dontSendNotification);
  label.setJustificationType(juce::Justification::centred);
  label.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
  label.setColour(juce::Label::textColourId, nam::ui::Colours::textSecondary);
}

void TopBarComponent::ConfigureValueLabel(juce::Label& label) {
  label.setJustificationType(juce::Justification::centred);
  label.setFont(juce::Font(juce::FontOptions(14.5f)));
  label.setColour(juce::Label::textColourId, nam::ui::Colours::textTertiary);
}

void TopBarComponent::ConfigureKnob(juce::Slider& knob) {
  knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
}

TopBarComponent::TopBarComponent() {
  ConfigureTag(mInput.tag, "Input");
  ConfigureTag(mOutput.tag, "Output");
  ConfigureValueLabel(mInput.value);
  ConfigureValueLabel(mOutput.value);
  ConfigureKnob(mInput.knob);
  ConfigureKnob(mOutput.knob);

  addAndMakeVisible(mInput.tag);
  addAndMakeVisible(mInput.knob);
  addAndMakeVisible(mInput.value);
  addAndMakeVisible(mOutput.tag);
  addAndMakeVisible(mOutput.knob);
  addAndMakeVisible(mOutput.value);

  mInput.knob.onValueChange = [this]() { UpdateValueLabel(mInput); };
  mOutput.knob.onValueChange = [this]() { UpdateValueLabel(mOutput); };

  UpdateValueLabel(mInput);
  UpdateValueLabel(mOutput);
}

void TopBarComponent::UpdateValueLabel(KnobColumn& knobColumn) {
  knobColumn.value.setText(juce::String(knobColumn.knob.getValue(), 1) + " dB",
                           juce::dontSendNotification);
}

void TopBarComponent::paint(juce::Graphics& g) {
  g.fillAll(nam::ui::Colours::topBarBackground);

  g.setColour(nam::ui::Colours::hairline);
  g.fillRect(getLocalBounds().removeFromBottom(1));

  juce::AttributedString title;
  title.setJustification(juce::Justification::centred);
  const auto titleFont = juce::Font(juce::FontOptions(32.0f, juce::Font::bold));
  title.append("Parametric ", titleFont, nam::ui::Colours::textPrimary);
  title.append("NAM", titleFont, nam::ui::Colours::accent);
  title.draw(g, mTitleBounds);
}

void TopBarComponent::LayoutColumn(juce::Rectangle<int> column, KnobColumn& knobColumn) {
  auto bounds = column.reduced(6, kColumnVerticalPadding);
  knobColumn.tag.setBounds(bounds.removeFromTop(kTagHeight));
  bounds.removeFromTop(kColumnGap);
  knobColumn.knob.setBounds(bounds.removeFromTop(kKnobSize).withSizeKeepingCentre(kKnobSize, kKnobSize));
  bounds.removeFromTop(kColumnGap);
  knobColumn.value.setBounds(bounds.removeFromTop(kValueHeight));
}

void TopBarComponent::resized() {
  auto bounds = getLocalBounds();
  LayoutColumn(bounds.removeFromLeft(kKnobColumnWidth), mInput);
  LayoutColumn(bounds.removeFromRight(kKnobColumnWidth), mOutput);
  mTitleBounds = bounds.toFloat();
}
