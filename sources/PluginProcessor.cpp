#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withInput ("Sidechain", juce::AudioChannelSet::mono(), false)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
                    parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
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

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("leftPPQ", "Left PPQ", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rightPPQ", "Right PPQ", 0.0f, 1.0f, 0.8f));

    return { params.begin(), params.end() };
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

    // Initialize the delay line
    int maxDelaySamples = static_cast<int>(sampleRate / audioPluginCutOffFrequency); // Maximum delay in samples
    delayLine.reset(); // Reset the delay line
    delayLine.prepare({ sampleRate, (juce::uint32)samplesPerBlock, 1 }); // Prepare the delay line
    delayLine.setMaximumDelayInSamples(maxDelaySamples); // Set maximum delay size
    delayLine.setDelay(0.0f); // Start with no delay

    // Initialize the analysis buffer
    const int analysisBufferSize = juce::nextPowerOfTwo(maxDelaySamples); // Size of the analysis buffer
    analysisBuffer.setSize(numChannels, analysisBufferSize); // Allocate the analysis buffer
    analysisBuffer.clear(); // Clear the analysis buffer to avoid garbage values
    analysisBufferWritePos = 0; // Reset the write position for the analysis buffer

    // Retrieve and store parameter pointers
    leftPPQBound  = parameters.getRawParameterValue("leftPPQ"); // Pointer to the left PPQ parameter
    rightPPQBound = parameters.getRawParameterValue("rightPPQ"); // Pointer to the right PPQ parameter
}

void AudioPluginAudioProcessor::releaseResources()
{
    displayBuffer.clear();
    analysisBuffer.clear();
    delayLine.reset();
    leftPPQBound = nullptr;
    rightPPQBound = nullptr;
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    clearExtraOutputChannels(buffer);

    auto input = getBusBuffer(buffer, true, 0);
    auto sidechain = getBusBuffer(buffer, true, 1);
    auto output = getBusBuffer(buffer, false, 0);

    processAudio(input, sidechain, output);
    updateUI(input, sidechain);

    // Compute new delay if host is playing
    if (auto* playhead = getPlayHead())
    {
        if (auto position = playhead->getPosition())
        {
            if (auto isPlaying = position->getIsPlaying())
            {
                if (auto ppq = position->getPpqPosition())
                {
                    double fractionalBeat = *ppq - std::floor(*ppq);
                    if (fractionalBeat > *leftPPQBound && fractionalBeat < *rightPPQBound && leftPPQBound != nullptr && rightPPQBound != nullptr)
                    {
                        int delay = findDelay(input, sidechain);
                        delaySamples.store(delay);
                        updateDelay(delay);
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
}

//==============================================================================
void AudioPluginAudioProcessor::processAudio(juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& sidechain, juce::AudioBuffer<float>& output)
{
    // Convert stereo to mono
    stereoToMono(input);
    stereoToMono(sidechain);

    // Delay input
    auto* channelData = input.getWritePointer(0);
    for (int i = 0; i < input.getNumSamples(); ++i)
    {
        float inputSample = channelData[i];
        float delayedSample = delayLine.popSample(0);
        delayLine.pushSample(0, inputSample);
        channelData[i] = delayedSample;
    }

    // Copy mono input to stereo output
    copyBuffer(input, 0, output, 0, 0, output.getNumSamples());
    copyBuffer(input, 0, output, 1, 0, output.getNumSamples());
}

void AudioPluginAudioProcessor::updateUI(juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& sidechain)
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
                copyBuffer(input, 0, displayBuffer, 0, index, input.getNumSamples(), true);
                copyBuffer(sidechain, 0, displayBuffer, 1, index, sidechain.getNumSamples(), true);
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

void AudioPluginAudioProcessor::copyBuffer(const juce::AudioBuffer<float>& src, int srcChannel,
                      juce::AudioBuffer<float>& dst, int dstChannel,
                      int writeStartIndex, int numSamples,
                      bool wrapAround)
{
    int destLength = dst.getNumSamples();

    // Safety checks
    if (srcChannel >= src.getNumChannels() ||
        dstChannel >= dst.getNumChannels() ||
        numSamples <= 0 || destLength <= 0)
        return;

    if (!wrapAround)
    {
        int samplesToCopy = std::min(numSamples, destLength - writeStartIndex);
        dst.copyFrom(dstChannel, writeStartIndex, src, srcChannel, 0, samplesToCopy);
        return;
    }

    // Wrap-around case
    int firstChunk = std::min(numSamples, destLength - writeStartIndex);
    int secondChunk = numSamples - firstChunk;

    // Copy first part
    if (firstChunk > 0)
        dst.copyFrom(dstChannel, writeStartIndex, src, srcChannel, 0, firstChunk);

    // Copy wrapped part to beginning
    if (secondChunk > 0)
        dst.copyFrom(dstChannel, 0, src, srcChannel, firstChunk, secondChunk);
}

int AudioPluginAudioProcessor::findDelay(juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& sidechain)
{
    copyBuffer(sidechain, 0, analysisBuffer, 0, analysisBufferWritePos, input.getNumSamples(), true);
    copyBuffer(input, 0, analysisBuffer, 1, analysisBufferWritePos, input.getNumSamples(), true);
    analysisBufferWritePos = (analysisBufferWritePos + input.getNumSamples()) % analysisBuffer.getNumSamples();

    const int numSamples = analysisBuffer.getNumSamples();
    const float* ref = analysisBuffer.getReadPointer(0);
    const float* target = analysisBuffer.getReadPointer(1);
    return crossCorrelation(ref, target, numSamples, numSamples, crossCorrelationStepSize);
}

void AudioPluginAudioProcessor::updateDelay(int delay)
{
    if (std::abs(delay) > (delayToleranceMs * getSampleRate() / 1000.0f))
    {
        // Gradient descent
        float currentDelay = delayLine.getDelay();
        float error = delay - currentDelay;
        float newDelay = currentDelay + learningRate.load() * error;

        // Ensure the new delay is within bounds
        newDelay = static_cast<float>(static_cast<int>(newDelay) % delayLine.getMaximumDelayInSamples());
        //newDelay = std::clamp(newDelay, 0.0f, static_cast<float>(delayLine.getMaximumDelayInSamples()));

        // Update the delay line with the new delay
        delayLine.setDelay(newDelay);
    }
}

int AudioPluginAudioProcessor::crossCorrelation(const float* ref, const float* target, int numSamples, int maxLagSamples, int stepSize)
{
    int bestLag = 0;
    float bestCorrelation = -std::numeric_limits<float>::infinity();

    for (int lag = 0; lag <= maxLagSamples; lag += stepSize)
    {
        float sum = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            int tgtIndex = i + lag;
            if (tgtIndex < numSamples)
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

int AudioPluginAudioProcessor::peakAlignment(const float* ref, const float* target, int numSamples)
{
    int refMaxIdx = 0;
    int targetMaxIdx = 0;
    float refMax = -std::numeric_limits<float>::infinity();
    float targetMax = -std::numeric_limits<float>::infinity();

    for (int i = 0; i < numSamples; ++i)
    {
        if (ref[i] > refMax)
        {
            refMax = ref[i];
            refMaxIdx = i;
        }

        if (target[i] > targetMax)
        {
            targetMax = target[i];
            targetMaxIdx = i;
        }
    }

    return targetMaxIdx - refMaxIdx;
}

void AudioPluginAudioProcessor::stereoToMono(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels <= 1 || numSamples == 0)
        return; // already mono or empty

    // Sum all channels into the first channel
    for (int ch = 1; ch < numChannels; ++ch)
        buffer.addFrom(0, 0, buffer, ch, 0, numSamples);

    // Average the result to prevent gain increase
    buffer.applyGain(0, 0, numSamples, 1.0f / (float) numChannels);

    // Optional: clear other channels
    for (int ch = 1; ch < numChannels; ++ch)
        buffer.clear(ch, 0, numSamples);
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
    // Save the state of the parameters (including leftPPQ and rightPPQ)
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore the state of the parameters (including leftPPQ and rightPPQ)
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (parameters.state.getType()))
    {
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
