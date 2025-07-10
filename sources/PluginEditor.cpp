#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), visualiser (2) // stereo
{
    juce::ignoreUnused (processorRef);
    
    setSize (600, 300);
    visualiser.setBufferSize(512);            // Internal ring buffer size
    visualiser.setSamplesPerBlock(128);        // Expected block size from processBlock
    visualiser.setRepaintRate(60);            // Refresh rate in Hz
    visualiser.setColours(juce::Colours::black, juce::Colours::lime);
    addAndMakeVisible(visualiser);

    startTimerHz(60); // Poll the FIFO at 60Hz
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void AudioPluginAudioProcessorEditor::resized()
{
    visualiser.setBounds(getLocalBounds());
}

void AudioPluginAudioProcessorEditor::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    visualiser.pushBuffer(buffer);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    int start1, size1, start2, size2;
    processorRef.fifo.prepareToRead(1, start1, size1, start2, size2);
    if (size1 > 0)
    {
        pushBuffer(processorRef.buffers[static_cast<std::size_t>(start1)]);
        processorRef.fifo.finishedRead(1);
    }
}