#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AboutOverlayComponent final : public juce::Component {
 public:
  AboutOverlayComponent();

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  bool keyPressed(const juce::KeyPress&) override;

  void Show();
  void Hide();

 private:
  static juce::File FindThirdPartyNoticesFile();

  juce::Rectangle<int> mCardBounds;
  juce::Rectangle<int> mTitleBounds;
  juce::Rectangle<int> mVersionBounds;
  juce::Rectangle<int> mCopyrightBounds;
  juce::Rectangle<int> mDividerBounds;
  juce::Rectangle<int> mSectionLabelBounds;

  juce::TextButton mCloseButton{"x"};
  juce::HyperlinkButton mRepoLink{"github.com/phillipmself/NamParametricPlugin",
                                  juce::URL("https://github.com/phillipmself/NamParametricPlugin")};
  juce::HyperlinkButton mNoticesLink{"Third-Party Notices", juce::URL()};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutOverlayComponent)
};
