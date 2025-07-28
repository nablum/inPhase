#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (400, 300);
    startTimerHz (30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    const auto& buffer = processorRef.getDisplayBuffer();
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    const int width  = getWidth();
    const int height = getHeight();
    const float midY = height / 2.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        juce::Path waveform;
        waveform.startNewSubPath(0, midY);

        for (int x = 0; x < width; ++x)
        {
            int bufferIndex = juce::jmap(x, 0, width, 0, numSamples - 1);
            float sample = samples[bufferIndex];

            float y = juce::jmap(sample, -1.0f, 1.0f, (float)height, 0.0f);
            waveform.lineTo((float)x, y);
        }

        g.setColour(getChannelColour(ch, numChannels));
        g.strokePath(waveform, juce::PathStrokeType(1.5f));
    }
}

void AudioPluginAudioProcessorEditor::resized()
{
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    repaint();
}