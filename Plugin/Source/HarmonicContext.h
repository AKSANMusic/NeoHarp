#pragma once

#include <vector>

//==============================================================================
enum class TransitionMode
{
    HARD_CUT,       // Release all RH voices immediately on chord change
    LEGATO_RETRIG,  // Release old pitches, immediately trigger same input keys at new mapped pitches
    SUSTAIN_HOLD    // Keep current output notes sounding until natural release, apply new mapping on next attacks
};

//==============================================================================
#include <array>

struct Scale
{
    int rootMidi { 60 };
    std::array<float, 24> intervalsCents {}; // fixed size container avoids allocation on audio thread
    size_t numIntervals { 0 };               // track how many intervals are actually active

    // Copy construction needs to be safe for lock-free FIFO usage
    Scale() = default;
    Scale(const Scale& other) = default;
    Scale& operator=(const Scale& other) = default;
};

//==============================================================================
// Holds the current state of the harmonic logic.
// This struct will be passed lock-free from the UI/Main Thread to the Audio Thread.
struct HarmonicContext
{
    Scale scale;
    TransitionMode transitionMode { TransitionMode::SUSTAIN_HOLD };

    HarmonicContext() = default;
    HarmonicContext(const HarmonicContext& other) = default;
    HarmonicContext& operator=(const HarmonicContext& other) = default;
};
