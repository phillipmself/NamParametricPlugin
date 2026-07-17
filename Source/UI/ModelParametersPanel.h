#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "NamParametricPluginProcessor.h"
#include "SegmentedSwitch.h"

class ModelParametersPanel final : public juce::Component {
 public:
  using RuntimeParameterInfo = NamParametricPluginAudioProcessor::RuntimeParameterInfo;

  ModelParametersPanel();

  void resized() override;

  void RebuildControls(const std::vector<RuntimeParameterInfo>& params,
                       const std::vector<double>& initialValues);
  void SetValue(size_t index, double value);
  [[nodiscard]] size_t GetControlCount() const { return mControls.size(); }

  std::function<void(size_t, double)> onValueChanged;

 private:
  struct ParamControl {
    bool isSwitch = false;
    std::unique_ptr<juce::Label> nameLabel;
    std::unique_ptr<juce::Slider> knob;
    std::unique_ptr<juce::Label> valueLabel;
    std::unique_ptr<SegmentedSwitch> switchControl;
  };

  void LayoutContent();

  juce::Label mEyebrow;
  juce::Viewport mViewport;
  juce::Component mContent;
  juce::Label mEmptyLabel;
  std::vector<ParamControl> mControls;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelParametersPanel)
};
