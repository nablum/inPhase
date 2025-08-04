#include <JuceHeader.h>
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

    // Number of channels
    int numChannels = getTotalNumOutputChannels();

    // Initialize the display buffer based on the sample rate and BPM
    double bpm = 120.0; // Example BPM, you might want to fetch this from host or a parameter later
    int samplesPerBeat = static_cast<int>((60.0 / bpm) * sampleRate); // Compute number of samples for 1 beat
    displayBuffer.setSize(numChannels, samplesPerBeat, false, true, true); // Allocate the buffer: numChannels x samplesPerBeat
    displayBuffer.clear(); // Clear buffer to avoid garbage values

    // Initialize the analysis buffer
    const int analysisBufferSize = 1024; // Size of the analysis buffer
    analysisBuffer.setSize(numChannels, analysisBufferSize); // Allocate the analysis buffer
    analysisBuffer.clear(); // Clear the analysis buffer to avoid garbage values
    analysisBufferWritePos = 0; // Reset the write position for the analysis buffer

    // Initialize the delay line
    maxDelaySamples = analysisBufferSize;
    delayLine.reset(); // Reset the delay line
    delayLine.prepare({ sampleRate, (juce::uint32)samplesPerBlock, 1 }); // Prepare the delay line
    delayLine.setMaximumDelayInSamples(maxDelaySamples); // Set maximum delay size
    delayLine.setDelay(0.0f); // Start with no delay
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

    // Apply fixed delay to channel 1 only
    float newTotalDelay = delayLine.getDelay() + delaySamples.load();
    if (newTotalDelay >= static_cast<float>(maxDelaySamples))
    {
        newTotalDelay = 0.0f;
    }
    else
    {
        newTotalDelay = std::clamp(newTotalDelay, 0.0f, static_cast<float>(maxDelaySamples));
    }
    delayLine.setDelay(newTotalDelay);
    auto* channelData = buffer.getWritePointer(1);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float inputSample = channelData[i];
        float delayedSample = delayLine.popSample(0);
        delayLine.pushSample(0, inputSample);
        channelData[i] = delayedSample;
    }

    // Compute delay sample between channels if playhead is available
    if (auto* playhead = getPlayHead())
    {
        if (auto position = playhead->getPosition())
        {
            if (auto isPlaying = position->getIsPlaying())
            {
                if (auto ppq = position->getPpqPosition())
                {
                    double fractionalBeat = *ppq - std::floor(*ppq); // Get the fractional part of the PPQ position
                    if (fractionalBeat > 0.2 && fractionalBeat < 0.8)
                    {
                        copyToBuffer(buffer, analysisBuffer, analysisBufferWritePos);
                        analysisBufferWritePos = (analysisBufferWritePos + buffer.getNumSamples()) % analysisBuffer.getNumSamples();
                        int delay = findDelayBetweenChannels(analysisBuffer, 0, 1, analysisBuffer.getNumSamples());
                        if (std::abs(delay - delaySamples.load()) > delayToleranceMs * getSampleRate() / 1000.0)
                        {
                            delaySamples.store(delay); 
                        }
                    }
                    else
                    {
                        analysisBuffer.clear();
                        analysisBufferWritePos = 0;
                    }
                }
            }
        }
    }

    updateUI(buffer);
    //processAudio(buffer);
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
                int index = getIndexFromPpq(*ppq);
                playheadIndex.store(index);
                copyToBuffer(buffer, displayBuffer, index);
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

int AudioPluginAudioProcessor::getIndexFromPpq(double ppqPosition) const
{
    double fractionalBeat = ppqPosition - std::floor(ppqPosition); // range [0.0, 1.0)

    int bufferLength = displayBuffer.getNumSamples();
    int index = static_cast<int>(fractionalBeat * bufferLength);

    // Safety clamp in case of rounding errors
    return std::clamp(index, 0, bufferLength - 1);
}

void AudioPluginAudioProcessor::copyToBuffer(const juce::AudioBuffer<float>& sourceBuffer,
                                             juce::AudioBuffer<float>& destinationBuffer,
                                             int writeStartIndex)
{
    int destLength = destinationBuffer.getNumSamples();
    int numSamples = sourceBuffer.getNumSamples();
    int numChannels = juce::jmin(sourceBuffer.getNumChannels(), destinationBuffer.getNumChannels());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* in = sourceBuffer.getReadPointer(ch);
        float* out = destinationBuffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            int writeIndex = (writeStartIndex + i) % destLength;
            out[writeIndex] = in[i];
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

    for (int lag = 0; lag <= maxLagSamples; ++lag) // Downsample by 10 - Cut-off: 48000Hz/2/10 = 2400Hz 
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
