#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NeoHarpAudioProcessorEditor::NeoHarpAudioProcessorEditor (NeoHarpAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), scaleBuilder(p)
{
    setSize (600, 400); // Larger size to fit grid and diagnostics

    strumSpeedSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    strumSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(strumSpeedSlider);

    addAndMakeVisible(scaleBuilder);

    strumSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "STRUM_SPEED", strumSpeedSlider);

    startTimerHz(30); // 30 FPS repaint for diagnostics and grid updates
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

    // Draw Diagnostics Sidebar
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(20, 100, 200, 150, 2);

    g.setFont(16.0f);
    g.setColour(juce::Colours::white);
    g.drawText("Diagnostics", 20, 110, 200, 20, juce::Justification::centred, false);

    g.setFont(12.0f);

    const auto& ctx = audioProcessor.getCanonicalContext();
    juce::String rootNote = "MIDI Note " + juce::String(ctx.scale.rootMidi);
    juce::String scaleSize = juce::String(ctx.scale.numIntervals) + " degrees";

    g.drawText("Active Root: " + rootNote, 30, 140, 180, 20, juce::Justification::left, false);
    g.drawText("Scale Size: " + scaleSize, 30, 160, 180, 20, juce::Justification::left, false);

    const auto& mapper = audioProcessor.getMapper();
    juce::String activeVoices = juce::String(mapper.getActiveVoiceCount());
    juce::String mpeAvailable = juce::String(mapper.getAvailableMpeChannelCount());

    g.drawText("RH Voices: " + activeVoices + " / 15", 30, 190, 180, 20, juce::Justification::left, false);
    g.drawText("MPE Pool: " + mpeAvailable, 30, 210, 180, 20, juce::Justification::left, false);
}

void NeoHarpAudioProcessorEditor::resized()
{
    strumSpeedSlider.setBounds(150, 60, 200, 20);
    scaleBuilder.setBounds(240, 40, 340, 340); // Align perfectly without overlap
}
