#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::clearExtraOutputChannels (juce::AudioBuffer<float>& buffer)
{
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    // Example BPM, you might want to fetch this from host or a parameter later
    double bpm = 120.0;

    // Compute number of samples for 1 beat
    int samplesPerBeat = static_cast<int>((60.0 / bpm) * sampleRate);

    // Choose the number of channels: same as your plugin's input/output
    int numChannels = getTotalNumOutputChannels();

    // Allocate the buffer: numChannels x samplesPerBeat
    displayBuffer.setSize(numChannels, samplesPerBeat, false, true, true);

    // Clear buffer to avoid garbage values
    displayBuffer.clear();
}

void AudioPluginAudioProcessor::releaseResources()
{
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    clearExtraOutputChannels(buffer);
    updateUI(buffer);
    processAudio(buffer);
    const int maxLagSamples = 1000; // adjust based on frequency content & accuracy needed
    int delay = findDelayBetweenChannels(buffer, 0, 1, maxLagSamples);
}

//==============================================================================
void AudioPluginAudioProcessor::processAudio(juce::AudioBuffer<float>& buffer)
{
    auto totalNumInputChannels = getTotalNumInputChannels();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        juce::ignoreUnused(channelData);
        // ..do something to the data...
    }
}

void AudioPluginAudioProcessor::updateUI(const juce::AudioBuffer<float>& buffer)
{
    if (auto* playhead = getPlayHead())
    {
        if (auto position = playhead->getPosition())
        {
            if (auto bpm = position->getBpm())
            {
                updateDisplayBufferIfNeeded(*bpm);
            }

            if (auto ppq = position->getPpqPosition())
            {
                int index = getDisplayBufferIndexFromPpq(*ppq);
                playheadIndex.store(index);
                copyToDisplayBuffer(buffer, index);
            }
        }
    }
}

void AudioPluginAudioProcessor::updateDisplayBufferIfNeeded(double bpm)
{
    constexpr double bpmTolerance = 0.01;
    if (bpm > 0.0 && std::abs(bpm - displayBufferBpm) > bpmTolerance)
    {
        displayBufferBpm = bpm;

        int samplesPerBeat = static_cast<int>((60.0 / bpm) * getSampleRate());
        int numChannels = getTotalNumOutputChannels();

        displayBuffer.setSize(numChannels, samplesPerBeat, false, true, true);
        displayBuffer.clear();
    }
}

int AudioPluginAudioProcessor::getDisplayBufferIndexFromPpq(double ppqPosition) const
{
    double fractionalBeat = ppqPosition - std::floor(ppqPosition); // range [0.0, 1.0)

    int bufferLength = displayBuffer.getNumSamples();
    int index = static_cast<int>(fractionalBeat * bufferLength);

    // Safety clamp in case of rounding errors
    return std::clamp(index, 0, bufferLength - 1);
}

void AudioPluginAudioProcessor::copyToDisplayBuffer(const juce::AudioBuffer<float>& sourceBuffer, int writeStartIndex)
{
    int displayLength = displayBuffer.getNumSamples();
    int numSamples = sourceBuffer.getNumSamples();
    int numChannels = sourceBuffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* in = sourceBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            int writeIndex = (writeStartIndex + i) % displayLength;
            displayBuffer.setSample(ch, writeIndex, in[i]);
        }
    }
}

int AudioPluginAudioProcessor::findDelayBetweenChannels(const juce::AudioBuffer<float>& buffer, int referenceChannel, int targetChannel, int maxLagSamples)
{
    const int numSamples = buffer.getNumSamples();

    const float* ref = buffer.getReadPointer(referenceChannel);
    const float* target = buffer.getReadPointer(targetChannel);

    int bestLag = 0;
    float bestCorrelation = -std::numeric_limits<float>::infinity();

    for (int lag = -maxLagSamples; lag <= maxLagSamples; ++lag)
    {
        float sum = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            int tgtIndex = i + lag;
            if (tgtIndex >= 0 && tgtIndex < numSamples)
                sum += ref[i] * target[tgtIndex];
        }

        if (sum > bestCorrelation)
        {
            bestCorrelation = sum;
            bestLag = lag;
        }
    }

    return bestLag;
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
