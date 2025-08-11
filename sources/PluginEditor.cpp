#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    delayLabel.setText("0.00 ms", juce::dontSendNotification);
    delayLabel.setFont(juce::Font(juce::FontOptions(16.f)));
    delayLabel.setAlpha(0.5f);
    delayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    delayLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(delayLabel);

    learningRateSlider.setSliderStyle(juce::Slider::LinearVertical);
    learningRateSlider.setColour(juce::Slider::thumbColourId, juce::Colours::greenyellow);
    learningRateSlider.setRange(Params::learningRateMin,
        Params::learningRateMax,Params::learningRateSensitivity);
    learningRateSlider.setValue(processorRef.getLearningRate());
    learningRateSlider.addListener(this);
    addAndMakeVisible(learningRateSlider);

    setSize (600, 300);
    startTimerHz (30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(juce::Colours::black);

    // Fill panel area (optional, for visual separation)
    //g.setColour(juce::Colours::darkgrey.darker(0.5f));
    //g.fillRect(0, 0, static_cast<int>(getWidth() * controlPanelRatio), getHeight());

    // Draw white separator bar between control panel and waveform area
    g.setColour(juce::Colours::whitesmoke);
    int separatorWidth = 4;
    int separatorX = waveformAreaRect.getX() - static_cast<int>(separatorWidth / 2.0f);
    g.fillRect(separatorX, 0, separatorWidth, getHeight());

    // Draw the current value below the slider
    auto sliderBounds = learningRateSlider.getBounds();
    float learningRateValue = (float)learningRateSlider.getValue();
    juce::String valueText = juce::String(learningRateValue, 2);
    int valueTextY = sliderBounds.getBottom() - 4;
    g.setFont(16.0f);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawFittedText(valueText,
        sliderBounds.getX(),
        valueTextY,
        sliderBounds.getWidth(),
        16,
        juce::Justification::centred,
        1);

    // Draw waveform and overlays in waveformAreaRect
    const auto& buffer = processorRef.getDisplayBuffer();
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const int width  = waveformAreaRect.getWidth();
    const int height = waveformAreaRect.getHeight();
    const float midY = waveformAreaRect.getY() + height / 2.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        juce::Path waveform;
        waveform.startNewSubPath(waveformAreaRect.getX(), midY);

        for (int x = 0; x < width; ++x)
        {
            int bufferIndex = juce::jmap(x, 0, width, 0, numSamples - 1);
            float sample = samples[bufferIndex];
            float y = juce::jmap(sample, -1.0f, 1.0f, (float)waveformAreaRect.getBottom(), (float)waveformAreaRect.getY());
            waveform.lineTo(waveformAreaRect.getX() + x, y);
        }

        g.setColour(getChannelColour(ch));
        g.strokePath(waveform, juce::PathStrokeType(1.5f));
    }

    // Draw playhead
    int index = processorRef.getPlayheadIndex();
    int bufferSize = buffer.getNumSamples();
    int cursorX = waveformAreaRect.getX() + static_cast<int>(static_cast<float>(index) / bufferSize * width);
    g.setColour(juce::Colours::greenyellow);
    g.drawLine((float)cursorX, (float)waveformAreaRect.getY(), (float)cursorX, (float)waveformAreaRect.getBottom(), 1.5f);

    // Draw computation area
    auto* leftPPQ = processorRef.getValueTreeState().getRawParameterValue("leftPPQ");
    auto* rightPPQ = processorRef.getValueTreeState().getRawParameterValue("rightPPQ");
    if (leftPPQ && rightPPQ)
    {
        float leftX = waveformAreaRect.getX() + *leftPPQ * width;
        float rightX = waveformAreaRect.getX() + *rightPPQ * width;

        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawLine(leftX, (float)waveformAreaRect.getY(), leftX, (float)waveformAreaRect.getBottom(), 2.0f);
        g.drawLine(rightX, (float)waveformAreaRect.getY(), rightX, (float)waveformAreaRect.getBottom(), 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(leftX, (float)waveformAreaRect.getY(), rightX - leftX, (float)height);
    }
}

void AudioPluginAudioProcessorEditor::resized()
{
    // Control panel area
    auto total = getLocalBounds();
    auto panelArea = total.removeFromLeft(static_cast<int>(total.getWidth() * controlPanelRatio));

    // Vertical slider in the center of the panel area
    int sliderWidth = 30;
    int sliderHeight = panelArea.getHeight() - 40;
    int sliderX = panelArea.getCentreX() - sliderWidth / 2;
    int sliderY = panelArea.getY() + (panelArea.getHeight() - sliderHeight) / 2;
    learningRateSlider.setBounds(sliderX, sliderY, sliderWidth, sliderHeight);

    // Store waveform area for paint()
    waveformAreaRect = total;

    // Place delayLabel in the bottom-right corner of waveformAreaRect
    int labelWidth = 180;
    int labelHeight = 24;
    int labelX = waveformAreaRect.getRight() - labelWidth - 8;
    int labelY = waveformAreaRect.getBottom() - labelHeight - 8;
    delayLabel.setBounds(labelX, labelY, labelWidth, labelHeight);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    int delay = processorRef.getDelaySamples();
    double ms = 1000.0 * delay / processorRef.getSampleRate();
    delayLabel.setText(juce::String(ms, 2) + " ms", juce::dontSendNotification);
    repaint();
}

void AudioPluginAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    auto localX = juce::jlimit(0.0f, (float)waveformAreaRect.getWidth(), 
                                (float)event.x - waveformAreaRect.getX());
    float fractional = localX / (float)waveformAreaRect.getWidth();

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
    auto localX = event.x - waveformAreaRect.getX(); // convert to waveform area coords
    float width = (float)waveformAreaRect.getWidth();

    auto* leftPPQ = processorRef.getValueTreeState().getRawParameterValue("leftPPQ");
    auto* rightPPQ = processorRef.getValueTreeState().getRawParameterValue("rightPPQ");

    if (leftPPQ != nullptr && rightPPQ != nullptr)
    {
        float leftX = *leftPPQ * width;
        float rightX = *rightPPQ * width;

        if (std::abs(localX - leftX) < barGrabRadius)
            draggingLeft = true;
        else if (std::abs(localX - rightX) < barGrabRadius)
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
