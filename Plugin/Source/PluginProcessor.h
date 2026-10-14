#pragma once

#include <JuceHeader.h>
#include "HarmonicContext.h"
#include "PitchMapper.h"

//==============================================================================
class NeoHarpAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    NeoHarpAudioProcessor();
    ~NeoHarpAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Public method for UI to update context
    void pushNewHarmonicContext (const HarmonicContext& newContext);

private:
    // Lock-free queue for receiving state updates from the message thread
    juce::AbstractFifo contextFifo { 16 };
    HarmonicContext contextBuffer[16];

    // The current state actively used in processBlock
    HarmonicContext currentContext;

    // Core Engine Mapper
    PitchMapper mapper;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoHarpAudioProcessor)
};
