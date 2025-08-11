#include <JuceHeader.h>
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

    learningRateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    learningRateSlider.setRange(Params::learningRateMin,
        Params::learningRateMax,Params::learningRateSensitivity);
    learningRateSlider.setValue(processorRef.getLearningRate());
    learningRateSlider.addListener(this);
    addAndMakeVisible(learningRateSlider);

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

    // Draw vertical lines for delay bounds
    if (auto* leftPPQ = processorRef.getValueTreeState().getRawParameterValue("leftPPQ"))
    {
        float leftX = *leftPPQ * width;
        g.setColour(juce::Colours::green);
        g.drawLine(leftX, 0.0f, leftX, (float)height, 2.0f);
    }

    if (auto* rightPPQ = processorRef.getValueTreeState().getRawParameterValue("rightPPQ"))
    {
        float rightX = *rightPPQ * width;
        g.setColour(juce::Colours::red);
        g.drawLine(rightX, 0.0f, rightX, (float)height, 2.0f);
    }
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

    learningRateSlider.setBounds(10, 10, getWidth() - 20, 30);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    int delay = processorRef.getDelaySamples();
    double ms = 1000.0 * delay / processorRef.getSampleRate();
    delayLabel.setText("Delay: " + juce::String(delay) + " samples (" + juce::String(ms, 2) + " ms)", juce::dontSendNotification);
    repaint();
}

void AudioPluginAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    float x = juce::jlimit(0.0f, (float)getWidth(), (float)event.x);
    float fractional = x / (float)getWidth();

    auto* leftParam = processorRef.getValueTreeState().getParameter("leftPPQ");
    auto* rightParam = processorRef.getValueTreeState().getParameter("rightPPQ");
    auto* leftPPQ = processorRef.getValueTreeState().getRawParameterValue("leftPPQ");
    auto* rightPPQ = processorRef.getValueTreeState().getRawParameterValue("rightPPQ");

    if (draggingLeft && leftParam && rightPPQ)
    {
        // Clamp leftPPQ to be less than rightPPQ
        float newLeft = std::min(fractional, *rightPPQ - 0.01f); // 0.01f is a small margin
        leftParam->setValueNotifyingHost(newLeft);
    }
    else if (draggingRight && rightParam && leftPPQ)
    {
        // Clamp rightPPQ to be greater than leftPPQ
        float newRight = std::max(fractional, *leftPPQ + 0.01f); // 0.01f is a small margin
        rightParam->setValueNotifyingHost(newRight);
    }

    repaint();
}

void AudioPluginAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    float x = (float)event.x;
    float width = (float)getWidth();

    auto* leftPPQ = processorRef.getValueTreeState().getRawParameterValue("leftPPQ");
    auto* rightPPQ = processorRef.getValueTreeState().getRawParameterValue("rightPPQ");

    if (leftPPQ != nullptr && rightPPQ != nullptr)
    {
        float leftX = *leftPPQ * width;
        float rightX = *rightPPQ * width;

        if (std::abs(x - leftX) < barGrabRadius)
            draggingLeft = true;
        else if (std::abs(x - rightX) < barGrabRadius)
            draggingRight = true;
    }
}

void AudioPluginAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    draggingLeft = false;
    draggingRight = false;
}

void AudioPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &learningRateSlider)
    {
        processorRef.setLearningRate((float)learningRateSlider.getValue());
    }
}
