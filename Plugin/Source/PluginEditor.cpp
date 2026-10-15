#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NeoHarpAudioProcessorEditor::NeoHarpAudioProcessorEditor (NeoHarpAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);

    strumSpeedSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    strumSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(strumSpeedSlider);

    strumSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "STRUM_SPEED", strumSpeedSlider);

    startTimerHz(30); // 30 FPS repaint for diagnostics
}

NeoHarpAudioProcessorEditor::~NeoHarpAudioProcessorEditor()
{
    stopTimer();
}

void NeoHarpAudioProcessorEditor::timerCallback()
{
    repaint();
}

//==============================================================================
void NeoHarpAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("NeoHarp", getLocalBounds().removeFromTop(40), juce::Justification::centred, true);

    g.setFont(14.0f);
    g.drawText("Strum Speed (ms):", 20, 60, 150, 20, juce::Justification::left, false);

    // Draw Diagnostics
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(20, 100, 360, 150, 2);

    g.setFont(16.0f);
    g.setColour(juce::Colours::white);
    g.drawText("Diagnostics", 20, 110, 360, 20, juce::Justification::centred, false);

    g.setFont(14.0f);

    const auto& ctx = audioProcessor.getCurrentContext();
    juce::String rootNote = "MIDI Note " + juce::String(ctx.scale.rootMidi);
    juce::String scaleSize = juce::String(ctx.scale.numIntervals) + " degrees";

    g.drawText("Active Root: " + rootNote, 30, 140, 300, 20, juce::Justification::left, false);
    g.drawText("Scale Size: " + scaleSize, 30, 160, 300, 20, juce::Justification::left, false);

    const auto& mapper = audioProcessor.getMapper();
    juce::String activeVoices = juce::String(mapper.getActiveVoiceCount());
    juce::String mpeAvailable = juce::String(mapper.getAvailableMpeChannelCount());

    g.drawText("Active RH Voices: " + activeVoices + " / 15", 30, 190, 300, 20, juce::Justification::left, false);
    g.drawText("MPE Channels Available: " + mpeAvailable, 30, 210, 300, 20, juce::Justification::left, false);
}

void NeoHarpAudioProcessorEditor::resized()
{
    strumSpeedSlider.setBounds(150, 60, 200, 20);
}
