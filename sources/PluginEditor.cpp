#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (600, 300);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::cyan);

    if (waveform.isEmpty())
        return;

    const int width = getWidth();
    const int height = getHeight();
    const int numSamples = waveform.size();

    juce::Path path;
    path.startNewSubPath(0, height / 2);

    for (int i = 0; i < numSamples; ++i)
    {
        const float x = juce::jmap<float>(i, 0, numSamples - 1, 0, (float)width);
        const float y = height / 2 - waveform[i] * (height / 2);
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void AudioPluginAudioProcessorEditor::resized()
{
}

void AudioPluginAudioProcessorEditor::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float* channelData = buffer.getReadPointer(0); // mono: first channel

    waveform.resize(numSamples);
    for (int i = 0; i < numSamples; ++i)
        waveform.set(i, channelData[i]);

    repaint();
}
