#include "ModelParametersPanel.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "NamColours.h"

namespace {
constexpr int kEyebrowHeight = 24;
constexpr int kKnobCellWidth = 90;
constexpr int kSwitchCellMinWidth = 84;
constexpr int kCellHeight = 108;
constexpr int kColumnGap = 26;
constexpr int kRowGap = 18;
constexpr int kSidePadding = 24;
constexpr int kTopPadding = 10;
constexpr int kBottomPadding = 16;
constexpr int kKnobSize = 62;
constexpr int kLabelHeight = 18;
constexpr int kSwitchHeight = 34;
constexpr int kEmptyStateHeight = 60;

juce::String FormatParamValue(const double value) { return juce::String(value, 2); }

}  // namespace

ModelParametersPanel::ModelParametersPanel() {
  mEyebrow.setText("Model Parameters", juce::dontSendNotification);
  mEyebrow.setJustificationType(juce::Justification::centred);
  mEyebrow.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
  mEyebrow.setColour(juce::Label::textColourId, nam::ui::Colours::textTertiary);
  addAndMakeVisible(mEyebrow);

  mViewport.setViewedComponent(&mContent, false);
  mViewport.setScrollBarsShown(true, false);
  addAndMakeVisible(mViewport);

  mEmptyLabel.setText("No dynamic parameters for this model.", juce::dontSendNotification);
  mEmptyLabel.setJustificationType(juce::Justification::centred);
  mEmptyLabel.setFont(juce::Font(juce::FontOptions(14.0f)));
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
    control.nameLabel->setFont(juce::Font(juce::FontOptions(14.5f, juce::Font::bold)));
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
      control.valueLabel->setFont(juce::Font(juce::FontOptions(13.5f)));
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

int ModelParametersPanel::GetMinimumContentHeight() const {
  return kEyebrowHeight + (mControls.empty() ? kEmptyStateHeight
                                             : kTopPadding + kCellHeight + kBottomPadding);
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
    mEmptyLabel.setBounds(0, 0, contentWidth, kEmptyStateHeight);
    mContent.setSize(contentWidth, juce::jmax(kEmptyStateHeight, mViewport.getHeight()));
    return;
  }

  mEmptyLabel.setVisible(false);

  const int maxRowWidth = juce::jmax(1, contentWidth - 2 * kSidePadding);

  // First pass: greedily pack controls into rows, each row only knowing its total width.
  std::vector<std::vector<std::pair<ParamControl*, int>>> rows;
  std::vector<std::pair<ParamControl*, int>> currentRow;
  int currentRowWidth = 0;

  for (auto& control : mControls) {
    const int cellWidth = control.isSwitch
                              ? juce::jmax(kSwitchCellMinWidth, control.switchControl->getPreferredWidth())
                              : kKnobCellWidth;
    const int neededWidth = currentRow.empty() ? cellWidth : currentRowWidth + kColumnGap + cellWidth;

    if (!currentRow.empty() && neededWidth > maxRowWidth) {
      rows.push_back(currentRow);
      currentRow.clear();
      currentRowWidth = 0;
    }

    if (!currentRow.empty()) {
      currentRowWidth += kColumnGap;
    }
    currentRowWidth += cellWidth;
    currentRow.emplace_back(&control, cellWidth);
  }
  if (!currentRow.empty()) {
    rows.push_back(currentRow);
  }

  // Second pass: each row is centered independently within the available width.
  int y = kTopPadding;
  for (const auto& row : rows) {
    int rowWidth = -kColumnGap;
    for (const auto& [control, cellWidth] : row) {
      rowWidth += cellWidth + kColumnGap;
    }

    int x = (contentWidth - rowWidth) / 2;
    for (const auto& [control, cellWidth] : row) {
      control->nameLabel->setBounds(x, y, cellWidth, kLabelHeight);

      if (control->isSwitch) {
        const int switchWidth = juce::jmin(cellWidth, control->switchControl->getPreferredWidth());
        control->switchControl->setBounds(x + (cellWidth - switchWidth) / 2, y + kLabelHeight + 10,
                                          switchWidth, kSwitchHeight);
      } else {
        const int knobX = x + (cellWidth - kKnobSize) / 2;
        control->knob->setBounds(knobX, y + kLabelHeight + 6, kKnobSize, kKnobSize);
        control->valueLabel->setBounds(x, y + kLabelHeight + 6 + kKnobSize + 4, cellWidth, kLabelHeight);
      }

      x += cellWidth + kColumnGap;
    }

    y += kCellHeight + kRowGap;
  }

  mContent.setSize(contentWidth,
                   juce::jmax(y - kRowGap + kBottomPadding, mViewport.getHeight()));
}
