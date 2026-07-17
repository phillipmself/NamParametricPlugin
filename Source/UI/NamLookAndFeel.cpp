#include "NamLookAndFeel.h"

#include <cmath>

#include "NamColours.h"

namespace {
using nam::ui::Colours::accent;
using nam::ui::Colours::background;
using nam::ui::Colours::hairline;
using nam::ui::Colours::knobFaceInner;
using nam::ui::Colours::knobFaceOuter;
using nam::ui::Colours::textPrimary;
using nam::ui::Colours::track;
}  // namespace

NamLookAndFeel::NamLookAndFeel() {
  setColour(juce::ResizableWindow::backgroundColourId, background);
  setColour(juce::Label::textColourId, textPrimary);
  setColour(juce::TextButton::buttonColourId, nam::ui::Colours::topBarBackground);
  setColour(juce::TextButton::textColourOffId, textPrimary);
  setColour(juce::TextButton::textColourOnId, textPrimary);
  setColour(juce::ComboBox::backgroundColourId, nam::ui::Colours::topBarBackground);
  setColour(juce::ComboBox::textColourId, textPrimary);
  setColour(juce::ComboBox::outlineColourId, hairline);
  setColour(juce::Slider::textBoxTextColourId, textPrimary);
  setColour(juce::Slider::textBoxOutlineColourId, hairline);
  setColour(juce::Slider::trackColourId, track);
  setColour(juce::Slider::thumbColourId, accent);
}

void NamLookAndFeel::drawRotarySlider(juce::Graphics& g, const int x, const int y,
                                      const int width, const int height,
                                      const float sliderPosProportional,
                                      const float rotaryStartAngle, const float rotaryEndAngle,
                                      juce::Slider&) {
  const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height))
                          .reduced(2.0f);

  const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  const float centreX = bounds.getCentreX();
  const float centreY = bounds.getCentreY();
  const float trackRadius = radius - 2.0f;
  const float trackThickness = juce::jmax(2.0f, radius * 0.16f);
  const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

  juce::Path trackPath;
  trackPath.addCentredArc(centreX, centreY, trackRadius, trackRadius, 0.0f, rotaryStartAngle,
                          rotaryEndAngle, true);
  g.setColour(track);
  g.strokePath(trackPath, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

  if (angle > rotaryStartAngle + 0.001f) {
    juce::Path fillPath;
    fillPath.addCentredArc(centreX, centreY, trackRadius, trackRadius, 0.0f, rotaryStartAngle,
                           angle, true);
    g.setColour(accent);
    g.strokePath(fillPath, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
  }

  const float faceRadius = trackRadius - trackThickness * 1.6f;
  juce::ColourGradient gradient(knobFaceInner, centreX - faceRadius * 0.3f, centreY - faceRadius * 0.4f,
                                knobFaceOuter, centreX, centreY + faceRadius, false);
  g.setGradientFill(gradient);
  g.fillEllipse(centreX - faceRadius, centreY - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f);

  const float pointerInner = faceRadius * 0.35f;
  const float pointerOuter = faceRadius * 0.85f;
  const juce::Point<float> innerPoint(centreX + pointerInner * std::sin(angle),
                                      centreY - pointerInner * std::cos(angle));
  const juce::Point<float> outerPoint(centreX + pointerOuter * std::sin(angle),
                                      centreY - pointerOuter * std::cos(angle));
  g.setColour(accent);
  g.drawLine(juce::Line<float>(innerPoint, outerPoint), trackThickness * 0.55f);
}
