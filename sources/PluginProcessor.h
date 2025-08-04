#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    void clearExtraOutputChannels (juce::AudioBuffer<float>& buffer);
    void updateDisplayBufferIfNeeded(double bpm);
    int getIndexFromPpq(double ppq) const;
    const juce::AudioBuffer<float>& getDisplayBuffer() const { return displayBuffer; }
    int getPlayheadIndex() const { return playheadIndex.load(); }
    void copyToBuffer(const juce::AudioBuffer<float>& sourceBuffer, juce::AudioBuffer<float>& destinationBuffer, int writeStartIndex);
    void updateUI(const juce::AudioBuffer<float>& buffer);
    void processAudio(juce::AudioBuffer<float>& buffer);
    int findDelayBetweenChannels(const juce::AudioBuffer<float>& buffer, int referenceChannel, int targetChannel, int maxLagSamples);
    int getDelaySamples() const { return delaySamples.load(); }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    juce::AudioBuffer<float> displayBuffer;
    double displayBufferBpm = -1.0;
    std::atomic<int> playheadIndex { 0 };
    juce::AudioBuffer<float> analysisBuffer;
    int analysisBufferWritePos = 0;
    std::atomic<int> delaySamples { 0 };
    double delayToleranceMs = 1.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
