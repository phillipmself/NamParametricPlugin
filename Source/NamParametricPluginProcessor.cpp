#include "NamParametricPluginProcessor.h"

#include "NamParametricPluginEditor.h"

NamParametricPluginAudioProcessor::NamParametricPluginAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::mono(), true)
                               .withOutput("Output", juce::AudioChannelSet::mono(), true)),
      mValueTree(*this, nullptr, "PARAMS", CreateParameterLayout()) {}

void NamParametricPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void NamParametricPluginAudioProcessor::releaseResources() {}

bool NamParametricPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  return layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() &&
         layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void NamParametricPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  if (buffer.getNumChannels() == 0) {
    return;
  }

  const float inputGainDb = mValueTree.getRawParameterValue(ParamIDs::inputGainDb)->load();
  const float outputGainDb = mValueTree.getRawParameterValue(ParamIDs::outputGainDb)->load();
  const float totalGain = juce::Decibels::decibelsToGain(inputGainDb + outputGainDb);

  buffer.applyGain(0, 0, buffer.getNumSamples(), totalGain);
}

juce::AudioProcessorEditor* NamParametricPluginAudioProcessor::createEditor() {
  return new NamParametricPluginAudioProcessorEditor(*this);
}

juce::AudioProcessorValueTreeState::ParameterLayout
NamParametricPluginAudioProcessor::CreateParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  const juce::NormalisableRange<float> gainRange(-12.0f, 12.0f, 0.01f);
  const auto gainAttrs = juce::AudioParameterFloatAttributes().withLabel("dB");

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(ParamIDs::inputGainDb, 1), "Input Gain", gainRange, 0.0f, gainAttrs));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID(ParamIDs::outputGainDb, 1), "Output Gain", gainRange, 0.0f, gainAttrs));

  return {params.begin(), params.end()};
}

void NamParametricPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto state = mValueTree.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void NamParametricPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml == nullptr) {
    return;
  }

  auto tree = juce::ValueTree::fromXml(*xml);
  if (tree.isValid() && tree.hasType(mValueTree.state.getType())) {
    mValueTree.replaceState(tree);
  }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new NamParametricPluginAudioProcessor();
}
