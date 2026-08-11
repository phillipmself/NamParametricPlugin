#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "SegmentedSwitch.h"

class IrBarComponent final : public juce::Component {
 public:
  IrBarComponent();

  void paint(juce::Graphics&) override;
  void resized() override;

  juce::TextButton& getSelectButton() { return mSelectButton; }
  juce::TextButton& getClearButton() { return mClearButton; }
  SegmentedSwitch& getEnabledSwitch() { return mEnabledSwitch; }
  void SetIrInfo(bool isLoaded, const juce::String& text);

 private:
  juce::TextButton mSelectButton;
  juce::Label mNameLabel;
  juce::TextButton mClearButton;
  SegmentedSwitch mEnabledSwitch;
  bool mIsLoaded = false;
  juce::Rectangle<int> mDotBounds;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IrBarComponent)
};
