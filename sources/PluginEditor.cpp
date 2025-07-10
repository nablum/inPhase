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

    const int width = getWidth();
    const int height = getHeight();
    const int numChannels = waveformChannels.size();

    if (numChannels == 0)
        return;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto& waveform = waveformChannels[ch];
        const int numSamples = waveform.size();
        if (numSamples == 0)
            continue;

        // Use unique colors per channel with transparency
        auto color = juce::Colour::fromHSV((float)ch / numChannels, 0.8f, 0.9f, 0.8f);
        g.setColour(color);

        juce::Path path;
        path.startNewSubPath(0, height / 2);

        for (int i = 0; i < numSamples; ++i)
        {
            float x = juce::jmap<float>(i, 0, numSamples - 1, 0.0f, (float)width);
            float y = height / 2 - waveform[i] * (height / 2);
            path.lineTo(x, y);
        }

        g.strokePath(path, juce::PathStrokeType(2.0f));
    }
}

void AudioPluginAudioProcessorEditor::resized()
{
}

void AudioPluginAudioProcessorEditor::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    waveformChannels.clearQuick();
    waveformChannels.ensureStorageAllocated(numChannels);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        juce::Array<float> channelArray;
        channelArray.resize(numSamples);

        for (int i = 0; i < numSamples; ++i)
            channelArray.set(i, channelData[i]);

        waveformChannels.add(std::move(channelArray));
    }

    repaint();
}
