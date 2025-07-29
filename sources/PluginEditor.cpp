#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    delayLabel.setText("Delay: 0 samples (0.00 ms)", juce::dontSendNotification);
    delayLabel.setFont(juce::Font(juce::FontOptions(16.f)));
    delayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    delayLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(delayLabel);

    setSize (400, 300);
    startTimerHz (30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Clear the background
    g.fillAll(juce::Colours::black);

    // Get buffer info and dimensions
    const auto& buffer = processorRef.getDisplayBuffer();
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const int width  = getWidth();
    const int height = getHeight();
    const float midY = height / 2.0f;

    // Draw the waveform for each channel
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

    // Draw a vertical line at the current playhead position
    int index = processorRef.getPlayheadIndex();
    int bufferSize = processorRef.getDisplayBuffer().getNumSamples();
    int cursorX = static_cast<int>(static_cast<float>(index) / bufferSize * getWidth());
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawLine((float)cursorX, 0.0f, (float)cursorX, (float)getHeight(), 1.5f);
}

void AudioPluginAudioProcessorEditor::resized()
{    
    // Place label 10px from right and bottom edges
    const int labelWidth = 200;
    const int labelHeight = 30;
    delayLabel.setBounds(
        getWidth() - labelWidth - 10,
        getHeight() - labelHeight - 10,
        labelWidth,
        labelHeight
    );
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    int delay = processorRef.getDelaySamples();
    double ms = 1000.0 * delay / processorRef.getSampleRate();
    delayLabel.setText("Delay: " + juce::String(delay) + " samples (" + juce::String(ms, 2) + " ms)", juce::dontSendNotification);
    repaint();
}