#pragma once

#include <JuceHeader.h>
#include "HarmonicContext.h"
#include <array>
#include <cmath>

class PitchMapper
{
public:
    PitchMapper();
    ~PitchMapper() = default;

    // Pops state changes and handles hanging notes at a specific sample position
    void applyNewContext(const HarmonicContext& newContext, juce::MidiBuffer& outputBuffer, int samplePosition);

    // Processes an incoming MIDI message and appends the mapped results to the output buffer
    void processMessage(const juce::MidiMessage& message, int samplePosition, juce::MidiBuffer& outputBuffer);

    // Clears all state
    void reset();

    // Set zone configurations
    void setZones(int lhMin, int lhMax, int rhMin, int rhMax);

    // Diagnostics for UI
    int getActiveVoiceCount() const { return static_cast<int>(numActiveVoices); }
    int getAvailableMpeChannelCount() const { return static_cast<int>(numAvailableMpeChannels); }

private:
    struct ActiveVoice
    {
        bool isActive { false };
        int inputNote { 0 };
        int outChannel { 0 };
        int outPitch { 0 };
        int outPb { 0 };
    };

    HarmonicContext currentContext;

    // Zones
    int lhZoneMin { 21 };
    int lhZoneMax { 60 };
    int rhZoneMin { 60 };
    int rhZoneMax { 109 };

    // Voice tracking (map by input note 0-127)
    std::array<ActiveVoice, 128> activeVoices {};

    // MPE Channel Pool (channels 2-16 in JUCE 1-based indexing)
    std::array<int, 15> mpeChannelPool {};
    size_t numAvailableMpeChannels { 15 };

    // Voice stealing queue (keep track of oldest voices)
    std::array<int, 15> activeVoiceNotes {};
    size_t numActiveVoices { 0 };

    int allocateChannel(int inputNote, juce::MidiBuffer& outputBuffer, int samplePosition);
    void freeChannel(int inputNote);
    void releaseAllRhVoices(juce::MidiBuffer& outputBuffer, int samplePosition);

    // Pitch math
    void calculatePitch(int inputNote, int& outPitch, int& outPb);
};
