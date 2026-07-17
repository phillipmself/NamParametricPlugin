#include "SegmentedSwitch.h"

#include "NamColours.h"

namespace {
constexpr int kSegmentMinWidth = 44;
constexpr int kHorizontalPadding = 12;
}  // namespace

void SegmentedSwitch::setOptions(const juce::StringArray& options) {
  mOptions = options;
  mSelectedIndex = juce::jlimit(0, juce::jmax(0, mOptions.size() - 1), mSelectedIndex);
  repaint();
}

void SegmentedSwitch::setSelectedIndex(const int index, const juce::NotificationType notification) {
  const int clamped = juce::jlimit(0, juce::jmax(0, mOptions.size() - 1), index);
  if (clamped == mSelectedIndex) {
    return;
  }

  mSelectedIndex = clamped;
  repaint();

  if (notification != juce::dontSendNotification && onChange != nullptr) {
    onChange(mSelectedIndex);
  }
}

int SegmentedSwitch::getPreferredWidth() const {
  int width = 4;
  for (const auto& option : mOptions) {
    width += juce::jmax(kSegmentMinWidth, option.length() * 7 + kHorizontalPadding);
  }
  return width;
}

void SegmentedSwitch::paint(juce::Graphics& g) {
  if (mOptions.isEmpty()) {
    return;
  }

  const auto bounds = getLocalBounds().toFloat();
  const float outerRadius = bounds.getHeight() / 2.0f;

  g.setColour(nam::ui::Colours::topBarBackground);
  g.fillRoundedRectangle(bounds, outerRadius);
  g.setColour(nam::ui::Colours::hairline);
  g.drawRoundedRectangle(bounds.reduced(0.5f), outerRadius, 1.0f);

  const auto inner = bounds.reduced(2.0f);
  const float segmentWidth = inner.getWidth() / static_cast<float>(mOptions.size());
  const float innerRadius = inner.getHeight() / 2.0f;

  g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));

  for (int i = 0; i < mOptions.size(); ++i) {
    const auto segment =
        juce::Rectangle<float>(inner.getX() + segmentWidth * static_cast<float>(i), inner.getY(),
                               segmentWidth, inner.getHeight());

    if (i == mSelectedIndex) {
      g.setColour(nam::ui::Colours::accent);
      g.fillRoundedRectangle(segment, innerRadius);
      g.setColour(nam::ui::Colours::onAccent);
    } else {
      g.setColour(nam::ui::Colours::textTertiary);
    }

    g.drawText(mOptions[i], segment.toNearestInt(), juce::Justification::centred);
  }
}

void SegmentedSwitch::mouseUp(const juce::MouseEvent& event) {
  if (mOptions.isEmpty() || getWidth() <= 0) {
    return;
  }

  const float segmentWidth = static_cast<float>(getWidth()) / static_cast<float>(mOptions.size());
  const int index =
      juce::jlimit(0, mOptions.size() - 1, static_cast<int>(event.position.x / segmentWidth));
  setSelectedIndex(index, juce::sendNotification);
}
