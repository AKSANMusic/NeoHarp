#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class NeoHarpAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    NeoHarpAudioProcessorEditor (NeoHarpAudioProcessor&);
    ~NeoHarpAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    NeoHarpAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoHarpAudioProcessorEditor)
};
