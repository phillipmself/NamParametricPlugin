#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "NamParametricPluginProcessor.h"
#include "UI/NamLookAndFeel.h"
#include "UI/TopBarComponent.h"

class NamParametricPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer {
 public:
  explicit NamParametricPluginAudioProcessorEditor(NamParametricPluginAudioProcessor&);
  ~NamParametricPluginAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using RuntimeParameterInfo = NamParametricPluginAudioProcessor::RuntimeParameterInfo;

  struct RuntimeControlRow {
    juce::String name;
    bool isSwitch = false;
    std::unique_ptr<juce::Label> label;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ComboBox> choice;
  };

  static bool RuntimeParameterListsEqual(const std::vector<RuntimeParameterInfo>& lhs,
                                         const std::vector<RuntimeParameterInfo>& rhs);
  void UpdateRuntimeParameterControls();
  void UpdateRuntimeParameterValues();
  void RebuildRuntimeParameterControls(const std::vector<RuntimeParameterInfo>& params);
  double GetInitialRuntimeValue(size_t index, const RuntimeParameterInfo& param) const;
  void timerCallback() override;

  NamParametricPluginAudioProcessor& mProcessor;
  NamLookAndFeel mLookAndFeel;
  TopBarComponent mTopBar;
  juce::TextButton mLoadModelButton;
  juce::Label mModelPathLabel;
  juce::Label mStatusLabel;

  juce::Label mRuntimeSectionLabel;
  juce::Viewport mRuntimeViewport;
  juce::Component mRuntimeContent;
  juce::Label mRuntimeEmptyLabel;

  std::unique_ptr<juce::FileChooser> mModelChooser;

  std::unique_ptr<SliderAttachment> mInputAttachment;
  std::unique_ptr<SliderAttachment> mOutputAttachment;

  std::vector<RuntimeParameterInfo> mLastRuntimeParameters;
  std::vector<RuntimeControlRow> mRuntimeRows;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamParametricPluginAudioProcessorEditor)
};
