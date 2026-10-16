#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NeoHarpAudioProcessorEditor::NeoHarpAudioProcessorEditor (NeoHarpAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), scaleBuilder(p), qwertyHandler(p.getQwertyCollector())
{
    setSize (600, 430); // Larger size to fit grid, diagnostics, and QWERTY toggle

    strumSpeedSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    strumSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    addAndMakeVisible(strumSpeedSlider);

    addAndMakeVisible(scaleBuilder);

    addAndMakeVisible(qwertyToggle);
    qwertyToggle.onClick = [this]()
    {
        if (qwertyToggle.getToggleState())
        {
            setWantsKeyboardFocus(true);
            addKeyListener(&qwertyHandler);
            grabKeyboardFocus();
        }
        else
        {
            removeKeyListener(&qwertyHandler);
            setWantsKeyboardFocus(false);
            qwertyHandler.reset();
        }
    };

    strumSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "STRUM_SPEED", strumSpeedSlider);

    startTimerHz(30); // 30 FPS repaint for diagnostics and grid updates
}

NeoHarpAudioProcessorEditor::~NeoHarpAudioProcessorEditor()
{
    removeKeyListener(&qwertyHandler);
    stopTimer();
}

void NeoHarpAudioProcessorEditor::timerCallback()
{
    // Poll commands from QWERTY handler (e.g. scale select, octave shift)
    auto cmd = qwertyHandler.popCommand();
    while (cmd.type != QwertyCommand::Type::None)
    {
        if (cmd.type == QwertyCommand::Type::SetScale)
        {
            // For PoC, just setting standard scales to test command routing
            HarmonicContext newCtx = audioProcessor.getCanonicalContext();
            if (cmd.value == 0) // Major
                newCtx.scale.intervalsCents = { 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f };
            else if (cmd.value == 1) // Minor
                newCtx.scale.intervalsCents = { 0.0f, 200.0f, 300.0f, 500.0f, 700.0f, 800.0f, 1000.0f };
            // Push update
            audioProcessor.setHarmonicContext(newCtx);
            scaleBuilder.repaint();
        }
        cmd = qwertyHandler.popCommand();
    }

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

    // QWERTY Latch indicator
    g.setColour(qwertyHandler.isLatchEnabled() ? juce::Colours::green : juce::Colours::darkgrey);
    g.fillEllipse(30, 235, 10, 10);
    g.setColour(juce::Colours::white);
    g.drawText("Chord Latch [Space]", 45, 230, 150, 20, juce::Justification::left, false);
}

void NeoHarpAudioProcessorEditor::resized()
{
    qwertyToggle.setBounds(20, 15, 200, 24);
    strumSpeedSlider.setBounds(150, 60, 200, 20);
    scaleBuilder.setBounds(240, 40, 340, 340); // Align perfectly without overlap
}
