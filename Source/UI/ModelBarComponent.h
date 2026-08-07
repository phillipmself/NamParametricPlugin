#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ModelBarComponent final : public juce::Component {
 public:
  ModelBarComponent();

  void paint(juce::Graphics&) override;
  void resized() override;

  juce::TextButton& getSelectButton() { return mSelectButton; }
  juce::TextButton& getClearButton() { return mClearButton; }
  void SetModelInfo(bool isLoaded, const juce::String& text);

 private:
  juce::TextButton mSelectButton;
  juce::Label mNameLabel;
  juce::TextButton mClearButton;
  bool mIsLoaded = false;
  juce::Rectangle<int> mDotBounds;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelBarComponent)
};
