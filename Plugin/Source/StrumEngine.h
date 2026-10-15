#pragma once

#include <JuceHeader.h>
#include <array>

class StrumEngine
{
public:
    StrumEngine() = default;
    ~StrumEngine() = default;

    // Resets internal state
    void reset();

    // Updates sample rate (needed to convert time to samples)
    void prepareToPlay(double sampleRate);

    // Schedules a chord of input notes to be strummed based on a musical time value (e.g. 1/16th note).
    // Using intervalMs if > 0, otherwise falls back to a BPM-based musical interval calculation.
    void scheduleStrum(const int* notes, size_t count, double intervalMs, bool downStrum, int velocityStart, int velocityDecay, double currentBpm = 120.0, double musicalNoteDivision = 0.25);

    // Processes the block, emitting any due note-ons into the output buffer
    void processBlock(juce::MidiBuffer& outputBuffer, int numSamples);

private:
    struct ScheduledNote
    {
        bool isActive { false };
        int inputNote { 0 };
        int velocity { 0 };
        long long samplesRemaining { 0 };
    };

    double currentSampleRate { 44100.0 };

    // Max notes we can schedule (e.g. 10 finger strum across a keyboard should be plenty)
    static constexpr size_t maxScheduledNotes = 16;
    std::array<ScheduledNote, maxScheduledNotes> scheduledQueue {};
};
