#pragma once

#include <JuceHeader.h>
#include "HarmonicContext.h"
#include "PitchMapper.h"
#include "StrumEngine.h"

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

    // Public method for UI to update canonical context, serialize, and push to audio thread
    void setHarmonicContext (const HarmonicContext& newContext);

    // Get the canonical message-thread context
    const HarmonicContext& getCanonicalContext() const { return canonicalContext; }

    // Audio Processor Value Tree State for automation and preset saving
    juce::AudioProcessorValueTreeState apvts;

private:
    // Lock-free queue for receiving state updates from the message thread
    juce::AbstractFifo contextFifo { 64 };
    HarmonicContext contextBuffer[64];

    // The current state actively used in processBlock (audio thread copy)
    HarmonicContext currentContext;

    // Canonical message-thread copy of the state
    HarmonicContext canonicalContext;

    // Core Engine Mapper
    PitchMapper mapper;

    // Sample-accurate Strum Scheduler
    StrumEngine strumEngine;

    // Pre-allocated MIDI buffers for processBlock
    juce::MidiBuffer strumBuffer;
    juce::MidiBuffer processedBuffer;

    // Cached parameter pointers for RT safety
    std::atomic<float>* strumSpeedParam { nullptr };

public:
    // Read-only diagnostic access for the UI
    const PitchMapper& getMapper() const { return mapper; }

    // Creates the parameter layout for APVTS
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoHarpAudioProcessor)
};
