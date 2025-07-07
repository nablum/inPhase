#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    void pushBuffer(const juce::AudioBuffer<float>& buffer);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    juce::AudioVisualiserComponent visualiser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
