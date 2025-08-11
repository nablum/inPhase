#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer, public juce::Slider::Listener
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
    void sliderValueChanged(juce::Slider* slider) override;

private:
    void timerCallback() override;
    juce::Colour getChannelColour(int channelIndex)
    {
        switch (channelIndex)
        {
        case 0:
            return juce::Colours::greenyellow;
        case 1:
            return juce::Colours::white.withAlpha(0.5f);
        default:
            return juce::Colours::red;
        }
    }
    juce::Label delayLabel;
    float barGrabRadius = 5.0f;
    bool draggingLeft = false;
    bool draggingRight = false;
    juce::Slider learningRateSlider;
    juce::Rectangle<int> waveformAreaRect;
    float controlPanelRatio = 1.0f / 8.0f;
    AudioPluginAudioProcessor& processorRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
