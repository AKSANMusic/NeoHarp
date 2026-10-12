#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NeoHarpAudioProcessorEditor::NeoHarpAudioProcessorEditor (NeoHarpAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

NeoHarpAudioProcessorEditor::~NeoHarpAudioProcessorEditor()
{
}

//==============================================================================
void NeoHarpAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("NeoHarp Plugin - C++ Engine", getLocalBounds(), juce::Justification::centred, 1);
}

void NeoHarpAudioProcessorEditor::resized()
{
}
