#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class TopBarComponent final : public juce::Component {
 public:
  TopBarComponent();

  void paint(juce::Graphics&) override;
  void resized() override;

  juce::Slider& getInputSlider() { return mInputKnob; }
  juce::Slider& getOutputSlider() { return mOutputKnob; }

 private:
  struct KnobColumn {
    juce::Label tag;
    juce::Slider knob{juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox};
    juce::Label value;
  };

  static void ConfigureTag(juce::Label&, const juce::String& text);
  static void ConfigureValueLabel(juce::Label&);
  static void ConfigureKnob(juce::Slider&);
  static void LayoutColumn(juce::Rectangle<int> column, KnobColumn& knobColumn);
  void UpdateValueLabel(KnobColumn& knobColumn);

  KnobColumn mInput;
  KnobColumn mOutput;
  juce::Slider& mInputKnob = mInput.knob;
  juce::Slider& mOutputKnob = mOutput.knob;

  juce::Rectangle<float> mTitleBounds;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBarComponent)
};
