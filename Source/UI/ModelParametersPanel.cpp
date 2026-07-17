#include "ModelParametersPanel.h"

#include <algorithm>
#include <cmath>

#include "NamColours.h"

namespace {
constexpr int kEyebrowHeight = 20;
constexpr int kKnobCellWidth = 76;
constexpr int kSwitchCellMinWidth = 70;
constexpr int kCellHeight = 90;
constexpr int kColumnGap = 20;
constexpr int kRowGap = 12;
constexpr int kSidePadding = 20;
constexpr int kTopPadding = 6;
constexpr int kBottomPadding = 12;
constexpr int kKnobSize = 52;
constexpr int kLabelHeight = 14;
constexpr int kSwitchHeight = 26;

juce::String FormatParamValue(const double value) { return juce::String(value, 2); }

}  // namespace

ModelParametersPanel::ModelParametersPanel() {
  mEyebrow.setText("Model Parameters", juce::dontSendNotification);
  mEyebrow.setJustificationType(juce::Justification::centred);
  mEyebrow.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
  mEyebrow.setColour(juce::Label::textColourId, nam::ui::Colours::textTertiary);
  addAndMakeVisible(mEyebrow);

  mViewport.setViewedComponent(&mContent, false);
  mViewport.setScrollBarsShown(true, false);
  addAndMakeVisible(mViewport);

  mEmptyLabel.setText("No dynamic parameters for this model.", juce::dontSendNotification);
  mEmptyLabel.setJustificationType(juce::Justification::centred);
  mEmptyLabel.setColour(juce::Label::textColourId, nam::ui::Colours::textTertiary);
  mContent.addAndMakeVisible(mEmptyLabel);
}

void ModelParametersPanel::RebuildControls(const std::vector<RuntimeParameterInfo>& params,
                                           const std::vector<double>& initialValues) {
  mControls.clear();
  mContent.removeAllChildren();
  mContent.addAndMakeVisible(mEmptyLabel);

  for (size_t index = 0; index < params.size(); ++index) {
    const auto& param = params[index];
    const double initialValue = index < initialValues.size() ? initialValues[index] : 0.0;

    ParamControl control;
    control.isSwitch = param.IsSwitch();

    control.nameLabel = std::make_unique<juce::Label>();
    control.nameLabel->setText(param.name, juce::dontSendNotification);
    control.nameLabel->setJustificationType(juce::Justification::centred);
    control.nameLabel->setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    control.nameLabel->setColour(juce::Label::textColourId, nam::ui::Colours::textSecondary);
    mContent.addAndMakeVisible(*control.nameLabel);

    if (control.isSwitch) {
      juce::StringArray options;
      for (const auto& name : param.enumNames) {
        options.add(name);
      }

      control.switchControl = std::make_unique<SegmentedSwitch>();
      control.switchControl->setOptions(options);
      control.switchControl->setSelectedIndex(static_cast<int>(std::round(initialValue)),
                                              juce::dontSendNotification);

      control.switchControl->onChange = [this, index](int selectedIndex) {
        if (onValueChanged != nullptr) {
          onValueChanged(index, static_cast<double>(selectedIndex));
        }
      };

      mContent.addAndMakeVisible(*control.switchControl);
    } else {
      double minValue = param.minValue;
      double maxValue = param.maxValue;
      if (maxValue < minValue) {
        std::swap(minValue, maxValue);
      }
      if (juce::approximatelyEqual(minValue, maxValue)) {
        maxValue = minValue + 1.0;
      }

      control.knob = std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag,
                                                     juce::Slider::NoTextBox);
      const double interval = std::max(0.0001, (maxValue - minValue) / 1000.0);
      control.knob->setRange(minValue, maxValue, interval);
      control.knob->setValue(initialValue, juce::dontSendNotification);

      control.valueLabel = std::make_unique<juce::Label>();
      control.valueLabel->setText(FormatParamValue(initialValue), juce::dontSendNotification);
      control.valueLabel->setJustificationType(juce::Justification::centred);
      control.valueLabel->setFont(juce::Font(juce::FontOptions(11.0f)));
      control.valueLabel->setColour(juce::Label::textColourId, nam::ui::Colours::textTertiary);

      auto* knob = control.knob.get();
      auto* valueLabel = control.valueLabel.get();
      control.knob->onValueChange = [this, knob, valueLabel, index]() {
        valueLabel->setText(FormatParamValue(knob->getValue()), juce::dontSendNotification);
        if (onValueChanged != nullptr) {
          onValueChanged(index, knob->getValue());
        }
      };

      mContent.addAndMakeVisible(*control.knob);
      mContent.addAndMakeVisible(*control.valueLabel);
    }

    mControls.push_back(std::move(control));
  }

  resized();
}

void ModelParametersPanel::SetValue(const size_t index, const double value) {
  if (index >= mControls.size()) {
    return;
  }

  auto& control = mControls[index];
  if (control.isSwitch && control.switchControl != nullptr) {
    control.switchControl->setSelectedIndex(static_cast<int>(std::round(value)),
                                            juce::dontSendNotification);
  } else if (control.knob != nullptr && !juce::approximatelyEqual(control.knob->getValue(), value)) {
    control.knob->setValue(value, juce::dontSendNotification);
    if (control.valueLabel != nullptr) {
      control.valueLabel->setText(FormatParamValue(value), juce::dontSendNotification);
    }
  }
}

void ModelParametersPanel::resized() {
  auto bounds = getLocalBounds();
  mEyebrow.setBounds(bounds.removeFromTop(kEyebrowHeight));
  mViewport.setBounds(bounds);
  LayoutContent();
}

void ModelParametersPanel::LayoutContent() {
  const int contentWidth =
      juce::jmax(1, mViewport.getWidth() - mViewport.getScrollBarThickness());

  if (mControls.empty()) {
    mEmptyLabel.setVisible(true);
    mEmptyLabel.setBounds(0, 0, contentWidth, kCellHeight / 2);
    mContent.setSize(contentWidth, juce::jmax(kCellHeight / 2, mViewport.getHeight()));
    return;
  }

  mEmptyLabel.setVisible(false);

  int x = kSidePadding;
  int y = kTopPadding;

  for (auto& control : mControls) {
    const int cellWidth = control.isSwitch
                              ? juce::jmax(kSwitchCellMinWidth, control.switchControl->getPreferredWidth())
                              : kKnobCellWidth;

    if (x + cellWidth + kSidePadding > contentWidth && x > kSidePadding) {
      x = kSidePadding;
      y += kCellHeight + kRowGap;
    }

    control.nameLabel->setBounds(x, y, cellWidth, kLabelHeight);

    if (control.isSwitch) {
      control.switchControl->setBounds(x, y + kLabelHeight + 8, cellWidth, kSwitchHeight);
    } else {
      const int knobX = x + (cellWidth - kKnobSize) / 2;
      control.knob->setBounds(knobX, y + kLabelHeight + 4, kKnobSize, kKnobSize);
      control.valueLabel->setBounds(x, y + kLabelHeight + 4 + kKnobSize + 4, cellWidth, kLabelHeight);
    }

    x += cellWidth + kColumnGap;
  }

  mContent.setSize(contentWidth, juce::jmax(y + kCellHeight + kBottomPadding, mViewport.getHeight()));
}
