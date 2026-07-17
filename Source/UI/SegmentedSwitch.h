#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

class SegmentedSwitch final : public juce::Component {
 public:
  SegmentedSwitch() = default;

  void setOptions(const juce::StringArray& options);
  void setSelectedIndex(int index, juce::NotificationType notification);
  [[nodiscard]] int getSelectedIndex() const { return mSelectedIndex; }
  [[nodiscard]] int getPreferredWidth() const;

  void paint(juce::Graphics&) override;
  void mouseUp(const juce::MouseEvent&) override;

  std::function<void(int)> onChange;

 private:
  juce::StringArray mOptions;
  int mSelectedIndex = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedSwitch)
};
