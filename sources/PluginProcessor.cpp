#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() = default;

const juce::String AudioPluginAudioProcessor::getName() const { return JucePlugin_Name; }

bool AudioPluginAudioProcessor::acceptsMidi() const { return false; }
bool AudioPluginAudioProcessor::producesMidi() const { return false; }
bool AudioPluginAudioProcessor::isMidiEffect() const { return false; }
double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginAudioProcessor::getNumPrograms() { return 1; }
int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }
void AudioPluginAudioProcessor::setCurrentProgram (int) {}
const juce::String AudioPluginAudioProcessor::getProgramName (int) { return {}; }
void AudioPluginAudioProcessor::changeProgramName (int, const juce::String&) {}

void AudioPluginAudioProcessor::prepareToPlay (double, int) {}
void AudioPluginAudioProcessor::releaseResources() {}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    // Example passthrough
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    // Forward audio to the editor for visualization
    if (auto* editor = dynamic_cast<AudioPluginAudioProcessorEditor*> (getActiveEditor()))
        editor->pushBuffer(buffer);
}

bool AudioPluginAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void AudioPluginAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
