#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void pushBuffer(const juce::AudioBuffer<float>& buffer, int startSampleIndex);
    void timerCallback() override;

private:
    AudioPluginAudioProcessor& processorRef;
    juce::Array<juce::Array<float>> waveformChannels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
