#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), visualiser (2) // stereo
{
    juce::ignoreUnused (processorRef);
    
    setSize (600, 300);
    visualiser.setBufferSize(128);            // Internal ring buffer size
    visualiser.setSamplesPerBlock(16);        // Expected block size from processBlock
    visualiser.setRepaintRate(60);            // Refresh rate in Hz
    visualiser.setColours(juce::Colours::black, juce::Colours::lime);
    addAndMakeVisible(visualiser);
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
