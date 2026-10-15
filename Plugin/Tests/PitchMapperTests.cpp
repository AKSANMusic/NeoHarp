#include <catch2/catch_test_macros.hpp>
#include "PitchMapper.h"

TEST_CASE("PitchMapper: Basic C Major Mapping", "[remapping]")
{
    PitchMapper mapper;

    HarmonicContext ctx;
    ctx.scale.rootMidi = 60; // C4
    ctx.scale.numIntervals = 7;
    ctx.scale.intervalsCents = { 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f };

    juce::MidiBuffer buffer;
    mapper.applyNewContext(ctx, buffer, 0);
    buffer.clear();

    // Send NoteOn for C4 (60) in the RH Zone
    juce::MidiMessage msgOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
    mapper.processMessage(msgOn, 0, buffer);

    // Expected: pitch bend of 0 (unsigned 8192) and a NoteOn for C4 on MPE channel 2
    int numEvents = 0;
    for (const auto metadata : buffer)
    {
        auto m = metadata.getMessage();
        if (numEvents == 0)
        {
            REQUIRE(m.isPitchWheel());
            REQUIRE(m.getChannel() == 2);
            REQUIRE(m.getPitchWheelValue() == 8192); // 0 pitch bend
        }
        else if (numEvents == 1)
        {
            REQUIRE(m.isNoteOn());
            REQUIRE(m.getChannel() == 2);
            REQUIRE(m.getNoteNumber() == 60);
        }
        numEvents++;
    }
    REQUIRE(numEvents == 2);
}

TEST_CASE("PitchMapper: Microtonal Math (Maqam Rast)", "[remapping]")
{
    PitchMapper mapper;

    HarmonicContext ctx;
    ctx.scale.rootMidi = 60; // C4
    ctx.scale.numIntervals = 7;
    ctx.scale.intervalsCents = { 0.0f, 200.0f, 350.0f, 500.0f, 700.0f, 900.0f, 1050.0f };

    juce::MidiBuffer buffer;
    mapper.applyNewContext(ctx, buffer, 0);
    buffer.clear();

    // Play degree 2 (E half-flat) which is key 62 in the RH zone
    juce::MidiMessage msgOn = juce::MidiMessage::noteOn(1, 62, (juce::uint8)100);
    mapper.processMessage(msgOn, 0, buffer);

    int numEvents = 0;
    for (const auto metadata : buffer)
    {
        auto m = metadata.getMessage();
        if (numEvents == 0)
        {
            REQUIRE(m.isPitchWheel());
            REQUIRE(m.getChannel() == 2);
            // 350 cents from C4 is E4 (64) - 50 cents.
            // PB range is +/- 200 cents over 14 bits (8192 values per direction)
            // -50 cents = -2048. Unsigned = 8192 - 2048 = 6144
            REQUIRE(m.getPitchWheelValue() == 6144);
        }
        else if (numEvents == 1)
        {
            REQUIRE(m.isNoteOn());
            REQUIRE(m.getNoteNumber() == 64);
        }
        numEvents++;
    }
    REQUIRE(numEvents == 2);
}

TEST_CASE("PitchMapper: Hanging Note Prevention", "[tracking]")
{
    PitchMapper mapper;

    HarmonicContext ctx1;
    ctx1.scale.rootMidi = 60; // C Major
    ctx1.scale.numIntervals = 7;
    ctx1.scale.intervalsCents = { 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f };

    juce::MidiBuffer buffer;
    mapper.applyNewContext(ctx1, buffer, 0);
    buffer.clear();

    // User holds D4 (key 61 in RH Zone -> maps to D4 output pitch 62)
    juce::MidiMessage holdNote = juce::MidiMessage::noteOn(1, 61, (juce::uint8)100);
    mapper.processMessage(holdNote, 0, buffer);
    buffer.clear();

    // User hits a new LH chord (changes root to G2 = 43) via UI update
    HarmonicContext ctx2;
    ctx2.scale.rootMidi = 43;
    ctx2.scale.numIntervals = 7;
    ctx2.scale.intervalsCents = ctx1.scale.intervalsCents;

    // The mapper MUST output a NoteOff for the active D4
    mapper.applyNewContext(ctx2, buffer, 5); // sample position 5

    bool foundNoteOff = false;
    for (const auto metadata : buffer)
    {
        auto m = metadata.getMessage();
        if (m.isNoteOff() && m.getNoteNumber() == 62 && m.getChannel() == 2)
        {
            foundNoteOff = true;
            REQUIRE(metadata.samplePosition == 5);
        }
    }
    REQUIRE(foundNoteOff == true);
}

TEST_CASE("PitchMapper: Voice Stealing", "[tracking]")
{
    PitchMapper mapper;

    HarmonicContext ctx;
    ctx.scale.rootMidi = 60;
    ctx.scale.numIntervals = 7;
    ctx.scale.intervalsCents = { 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f };

    juce::MidiBuffer buffer;
    mapper.applyNewContext(ctx, buffer, 0);
    buffer.clear();

    // Exhaust MPE Pool (15 channels)
    for (int i = 0; i < 15; ++i)
    {
        mapper.processMessage(juce::MidiMessage::noteOn(1, 60 + i, (juce::uint8)100), 0, buffer);
    }
    buffer.clear();

    // Play 16th note, should steal the oldest (which was key 60 on channel 2)
    mapper.processMessage(juce::MidiMessage::noteOn(1, 75, (juce::uint8)100), 10, buffer);

    bool stoleNote = false;
    for (const auto metadata : buffer)
    {
        auto m = metadata.getMessage();
        // The first event should be NoteOff for the stolen voice
        if (m.isNoteOff() && m.getNoteNumber() == 60 && m.getChannel() == 2)
        {
            stoleNote = true;
            REQUIRE(metadata.samplePosition == 10);
            break;
        }
    }
    REQUIRE(stoleNote == true);
}
