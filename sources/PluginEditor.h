#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Colour getChannelColour(int channelIndex, int totalChannels)
    {
        float hue = static_cast<float>(channelIndex) / juce::jmax(1, totalChannels);
        return juce::Colour::fromHSV(hue, 0.8f, 0.9f, 1.0f); // hue, saturation, brightness, alpha
    }
    juce::Label delayLabel;
    float barGrabRadius = 5.0f;
    bool draggingLeft = false;
    bool draggingRight = false;
    AudioPluginAudioProcessor& processorRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
