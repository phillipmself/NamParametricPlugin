#include "AboutOverlayComponent.h"

#include <JuceHeader.h>

#include "NamColours.h"

namespace {
constexpr int kCardWidth = 380;
constexpr int kCardPadding = 22;
constexpr int kCloseButtonSize = 22;
constexpr int kTitleHeight = 28;
constexpr int kVersionHeight = 18;
constexpr int kCopyrightHeight = 19;
constexpr int kRepoLinkHeight = 19;
constexpr int kDividerGap = 12;
constexpr int kSectionLabelHeight = 17;
constexpr int kNoticesLinkHeight = 21;
constexpr int kRowGap = 4;
}  // namespace

juce::File AboutOverlayComponent::FindThirdPartyNoticesFile() {
  const auto exeFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

  // macOS/Windows VST3 and Standalone builds are Contents/MacOS/<exe> bundles;
  // the notices file is copied into Contents/Resources alongside them.
  const auto bundleResource =
      exeFile.getParentDirectory().getParentDirectory().getChildFile("Resources/THIRD-PARTY-NOTICES.md");
  if (bundleResource.existsAsFile()) {
    return bundleResource;
  }

  const auto flatSibling = exeFile.getSiblingFile("THIRD-PARTY-NOTICES.md");
  if (flatSibling.existsAsFile()) {
    return flatSibling;
  }

  return {};
}

AboutOverlayComponent::AboutOverlayComponent() {
  mCloseButton.onClick = [this]() { Hide(); };
  addAndMakeVisible(mCloseButton);

  mRepoLink.setFont(juce::Font(juce::FontOptions(13.5f)), false, juce::Justification::centredLeft);
  mRepoLink.setColour(juce::HyperlinkButton::textColourId, nam::ui::Colours::textSecondary);
  addAndMakeVisible(mRepoLink);

  mNoticesLink.setFont(juce::Font(juce::FontOptions(13.5f)), false, juce::Justification::centredLeft);
  mNoticesLink.setColour(juce::HyperlinkButton::textColourId, nam::ui::Colours::textPrimary);
  const auto noticesFile = FindThirdPartyNoticesFile();
  if (noticesFile.existsAsFile()) {
    mNoticesLink.setURL(juce::URL(noticesFile));
  } else {
    mNoticesLink.setButtonText("Third-Party Notices (file not found)");
    mNoticesLink.setEnabled(false);
  }
  addAndMakeVisible(mNoticesLink);

  setInterceptsMouseClicks(true, true);
  setWantsKeyboardFocus(true);
  setVisible(false);
}

void AboutOverlayComponent::Show() {
  setVisible(true);
  toFront(true);
  grabKeyboardFocus();
}

void AboutOverlayComponent::Hide() { setVisible(false); }

void AboutOverlayComponent::mouseDown(const juce::MouseEvent& event) {
  if (!mCardBounds.contains(event.getPosition())) {
    Hide();
  }
}

bool AboutOverlayComponent::keyPressed(const juce::KeyPress& key) {
  if (key == juce::KeyPress::escapeKey) {
    Hide();
    return true;
  }
  return false;
}

void AboutOverlayComponent::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black.withAlpha(0.55f));

  g.setColour(juce::Colour(0xff17181c));
  g.fillRoundedRectangle(mCardBounds.toFloat(), 8.0f);
  g.setColour(juce::Colour(0xff323339));
  g.drawRoundedRectangle(mCardBounds.toFloat().reduced(0.5f), 8.0f, 1.0f);

  juce::AttributedString title;
  title.setJustification(juce::Justification::centredLeft);
  const auto titleFont = juce::Font(juce::FontOptions(20.0f, juce::Font::bold));
  title.append("Parametric ", titleFont, nam::ui::Colours::textPrimary);
  title.append("NAM", titleFont, nam::ui::Colours::accent);
  title.draw(g, mTitleBounds.toFloat());

  g.setColour(nam::ui::Colours::textSecondary);
  g.setFont(juce::Font(juce::FontOptions(13.0f)));
  g.drawText("Version " + juce::String(ProjectInfo::versionString), mVersionBounds,
             juce::Justification::centredLeft);

  g.setColour(nam::ui::Colours::textSecondary);
  g.setFont(juce::Font(juce::FontOptions(14.0f)));
  g.drawText(juce::CharPointer_UTF8("\xc2\xa9 2026 Phillip Self"), mCopyrightBounds,
             juce::Justification::centredLeft);

  g.setColour(nam::ui::Colours::hairline);
  g.fillRect(mDividerBounds);

  g.setColour(nam::ui::Colours::textTertiary);
  g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
  g.drawText("THIRD-PARTY NOTICES", mSectionLabelBounds, juce::Justification::centredLeft);
}

void AboutOverlayComponent::resized() {
  const int cardHeight = kCardPadding * 2 + kTitleHeight + kRowGap + kVersionHeight + kRowGap +
                         kCopyrightHeight + kRowGap + kRepoLinkHeight + kDividerGap + 1 + kDividerGap +
                         kSectionLabelHeight + kRowGap + kNoticesLinkHeight;
  mCardBounds = juce::Rectangle<int>(0, 0, kCardWidth, cardHeight).withCentre(getLocalBounds().getCentre());

  mCloseButton.setBounds(mCardBounds.getRight() - kCardPadding - kCloseButtonSize + 6,
                         mCardBounds.getY() + kCardPadding - 4, kCloseButtonSize, kCloseButtonSize);

  auto content = mCardBounds.reduced(kCardPadding);
  mTitleBounds = content.removeFromTop(kTitleHeight);
  content.removeFromTop(kRowGap);
  mVersionBounds = content.removeFromTop(kVersionHeight);
  content.removeFromTop(kRowGap);
  mCopyrightBounds = content.removeFromTop(kCopyrightHeight);
  content.removeFromTop(kRowGap);
  mRepoLink.setBounds(content.removeFromTop(kRepoLinkHeight));
  content.removeFromTop(kDividerGap);
  mDividerBounds = content.removeFromTop(1);
  content.removeFromTop(kDividerGap);
  mSectionLabelBounds = content.removeFromTop(kSectionLabelHeight);
  content.removeFromTop(kRowGap);
  mNoticesLink.setBounds(content.removeFromTop(kNoticesLinkHeight));
}
