#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"

TEST_CASE("State Serialization: HarmonicContext Round Trip", "[serialization]")
{
    // Note: We're testing the headless PluginProcessor methods
    NeoHarpAudioProcessor processor;

    // Set up a unique microtonal context (e.g. Quarter tone scale)
    HarmonicContext customContext;
    customContext.scale.rootMidi = 62; // D
    customContext.scale.numIntervals = 3;
    customContext.scale.intervalsCents[0] = 0.0f;
    customContext.scale.intervalsCents[1] = 150.0f; // Neutral 2nd
    customContext.scale.intervalsCents[2] = 350.0f; // Neutral 3rd
    customContext.transitionMode = TransitionMode::LEGATO_RETRIG;

    processor.setHarmonicContext(customContext);

    // Get State Information (Save)
    juce::MemoryBlock savedData;
    processor.getStateInformation(savedData);

    REQUIRE(savedData.getSize() > 0);

    // Create a NEW processor instance to simulate loading a project
    NeoHarpAudioProcessor newProcessor;

    // Set State Information (Restore)
    newProcessor.setStateInformation(savedData.getData(), static_cast<int>(savedData.getSize()));

    // Verify restored state matches
    const auto& restoredContext = newProcessor.getCanonicalContext();

    REQUIRE(restoredContext.scale.rootMidi == 62);
    REQUIRE(restoredContext.scale.numIntervals == 3);
    REQUIRE(restoredContext.transitionMode == TransitionMode::LEGATO_RETRIG);

    REQUIRE(restoredContext.scale.intervalsCents[0] == 0.0f);
    REQUIRE(restoredContext.scale.intervalsCents[1] == 150.0f);
    REQUIRE(restoredContext.scale.intervalsCents[2] == 350.0f);
}
