#pragma once

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
    int findDelayBetweenChannels(const juce::AudioBuffer<float>& buffer, int referenceChannel, int targetChannel, int maxLagSamples);
    int crossCorrelation(const float* ref, const float* target, int numSamples, int maxLagSamples, int stepSize);
    int peakAlignment(const float* ref, const float* target, int numSamples);
    int getDelaySamples() const { return delaySamples.load(); }
    float getLeftPPQ() const { return leftPPQBound->load(); }
    float getRightPPQ() const { return rightPPQBound->load(); }
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void setLearningRate(float newValue) { learningRate.store(newValue); }
    float getLearningRate() const { return learningRate.load(); }

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
    int crossCorrelationStepSize = 4;
    std::atomic<int> delaySamples { 0 };
    float delayToleranceMs = 0.1f;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
    float audioPluginCutOffFrequency = 30.0f;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* leftPPQBound = nullptr;
    std::atomic<float>* rightPPQBound = nullptr;
    std::atomic<float> learningRate { 0.2f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
