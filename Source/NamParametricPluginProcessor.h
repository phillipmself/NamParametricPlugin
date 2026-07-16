#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "DSP/NamModelEngine.h"

class NamParametricPluginAudioProcessor final : public juce::AudioProcessor, private juce::Timer {
 public:
  struct ParamIDs {
    static constexpr const char* inputGainDb = "inputGainDb";
    static constexpr const char* outputGainDb = "outputGainDb";
  };

  struct RuntimeParameterInfo {
    juce::String name;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 0.0;
    std::vector<juce::String> enumNames;

    [[nodiscard]] bool IsSwitch() const { return !enumNames.empty(); }
  };

  NamParametricPluginAudioProcessor();
  ~NamParametricPluginAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "NAM Parametric Plugin"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  void LoadModelAsync(const juce::File& modelFile);
  juce::String GetStatusText() const;
  juce::String GetModelPath() const;
  bool HasModelLoaded() const;
  std::vector<RuntimeParameterInfo> GetRuntimeParameters() const;
  void SetRuntimeParameterValue(size_t index, double value);
  std::optional<double> GetRuntimeParameterValue(size_t index) const;

  juce::AudioProcessorValueTreeState mValueTree;

 private:
  // Non-audio writers serialize complete snapshots with an odd/even generation. Each value is
  // atomic, so a reader retry is data-race-free; the audio thread only performs bounded atomic
  // loads into preallocated storage before one full-vector SetParams() call.
  class RuntimeParameterMailbox {
   public:
    explicit RuntimeParameterMailbox(std::span<const float> initialValues);

    [[nodiscard]] size_t Size() const { return mSize; }
    void Publish(size_t index, float value);
    void PublishAll(std::span<const float> values);
    [[nodiscard]] std::optional<float> Read(size_t index) const;
    bool TryReadStable(std::span<float> destination, uint64_t& version) const;

   private:
    const size_t mSize;
    std::unique_ptr<std::atomic<float>[]> mValues;
    mutable std::mutex mWriterMutex;
    std::atomic<uint64_t> mVersion{0};
  };

  struct ProcessingState {
    std::unique_ptr<namparametric::dsp::NamModelEngine> model;
    std::shared_ptr<RuntimeParameterMailbox> mailbox;
    std::vector<float> pendingValues;
    uint64_t consumedVersion = 0;
  };

  struct RestoredRuntimeParameterValue {
    int index = -1;
    std::string name;
    float value = 0.0f;
  };

  struct AsyncLoadResult {
    bool success = false;
    uint64_t requestId = 0;
    std::unique_ptr<ProcessingState> processingState;
    juce::String message;
    juce::String loadedPath;
    std::vector<RuntimeParameterInfo> runtimeParameters;
    std::vector<RestoredRuntimeParameterValue> restoredValues;
  };

  struct LoadRequest {
    juce::File modelFile;
    std::vector<RestoredRuntimeParameterValue> restoredValues;
    uint64_t requestId = 0;
  };

  struct HostProcessingConfig {
    std::atomic<uint64_t> generation{0};
    std::atomic<double> sampleRate{48000.0};
    std::atomic<int> blockSize{512};
  };

  juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout();
  void StartModelLoad(const juce::File& modelFile,
                      std::vector<RestoredRuntimeParameterValue> restoredValues = {});
  void BeginModelLoadLocked(LoadRequest request);
  void CollectCompletedModelLoad();
  void ApplyRestoredValues(AsyncLoadResult& result);
  void StageModelClear();
  void TryPromoteStagedModel();
  void ApplyPendingRuntimeParameterChanges();
  void timerCallback() override;

  mutable std::mutex mLoadMutex;
  std::optional<std::future<AsyncLoadResult>> mLoadFuture;
  std::optional<LoadRequest> mQueuedLoadRequest;
  uint64_t mLatestLoadRequestId = 0;
  bool mClearModelStaged = false;
  // The audio thread only moves these owners. The message thread collects the future and destroys
  // retired models, keeping construction, prewarming, allocation, and destruction out of
  // processBlock().
  std::unique_ptr<ProcessingState> mActiveProcessingState;
  std::unique_ptr<ProcessingState> mStagedProcessingState;
  std::unique_ptr<ProcessingState> mRetiredProcessingState;

  juce::String mStatusText = "No model loaded";
  juce::String mModelPath;
  std::vector<RuntimeParameterInfo> mRuntimeParameters;
  std::shared_ptr<RuntimeParameterMailbox> mRuntimeParameterMailbox;

  std::vector<float> mInputScratch;
  std::vector<float> mOutputScratch;

  std::shared_ptr<HostProcessingConfig> mHostProcessingConfig =
      std::make_shared<HostProcessingConfig>();
  std::atomic<float>* mInputGainParam = nullptr;
  std::atomic<float>* mOutputGainParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamParametricPluginAudioProcessor)
};
