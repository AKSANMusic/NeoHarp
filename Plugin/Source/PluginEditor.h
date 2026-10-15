#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class NeoHarpAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    NeoHarpAudioProcessorEditor (NeoHarpAudioProcessor&);
    ~NeoHarpAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // Timer for repainting diagnostics
    void timerCallback() override;

private:
    NeoHarpAudioProcessor& audioProcessor;

    juce::Slider strumSpeedSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strumSpeedAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoHarpAudioProcessorEditor)
};
