#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "PluginProcessor.h"
#include <random>

// Mock AudioPlayHead to simulate DAW BPM changes
class MockPlayHead : public juce::AudioPlayHead
{
public:
    double currentBpm { 120.0 };

    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
    {
        juce::AudioPlayHead::PositionInfo info;
        info.setBpm(currentBpm);
        return info;
    }
};

TEST_CASE("Stress Test: ProcessBlock Fuzzing", "[stress]")
{
    NeoHarpAudioProcessor processor;
    MockPlayHead playHead;
    processor.setPlayHead(&playHead);

    processor.prepareToPlay(44100.0, 1024);

    juce::AudioBuffer<float> dummyAudio(2, 1024);
    juce::MidiBuffer midiIn;

    std::mt19937 rng(1337); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> bufferSizeDist(1, 1024);
    std::uniform_real_distribution<double> bpmDist(60.0, 220.0);
    std::uniform_int_distribution<int> rhKeyDist(60, 100);
    std::uniform_int_distribution<int> lhRootDist(36, 48);

    // Test 1000 consecutive process blocks
    for (int block = 0; block < 1000; ++block)
    {
        int bufSize = bufferSizeDist(rng);
        playHead.currentBpm = bpmDist(rng);
        dummyAudio.setSize(2, bufSize, false, true, true);
        midiIn.clear();

        // 5% chance to change scale and force NoteOffs
        if (block % 20 == 0)
        {
            HarmonicContext newCtx;
            newCtx.scale.rootMidi = lhRootDist(rng);
            newCtx.scale.numIntervals = 7;
            newCtx.scale.intervalsCents = { 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f };
            processor.setHarmonicContext(newCtx);
        }

        // Trigger random RH notes
        if (block % 3 == 0)
        {
            midiIn.addEvent(juce::MidiMessage::noteOn(1, rhKeyDist(rng), (juce::uint8)100), 0);
        }

        // Randomly release notes
        if (block % 5 == 0)
        {
            midiIn.addEvent(juce::MidiMessage::noteOff(1, rhKeyDist(rng)), 0);
        }

        // Trigger Strum (LH C1)
        if (block % 50 == 0)
        {
            midiIn.addEvent(juce::MidiMessage::noteOn(1, 24, (juce::uint8)100), 0);
        }

        processor.processBlock(dummyAudio, midiIn);
    }

    // Step 2: Flush the StrumEngine (let time pass)
    for (int i = 0; i < 50; ++i)
    {
        dummyAudio.setSize(2, 1024, false, true, true);
        midiIn.clear();
        processor.processBlock(dummyAudio, midiIn);
    }

    // Step 3: Send Note-Off for ALL RH zone keys to ensure we clear user input
    dummyAudio.setSize(2, 1024, false, true, true);
    midiIn.clear();
    for (int key = 60; key <= 109; ++key)
    {
        midiIn.addEvent(juce::MidiMessage::noteOff(1, key), 0);
    }
    processor.processBlock(dummyAudio, midiIn);

    // Step 4: One final empty block
    dummyAudio.setSize(2, 1024, false, true, true);
    midiIn.clear();
    processor.processBlock(dummyAudio, midiIn);

    // Assert absolute zero hanging notes
    auto& mapper = processor.getMapper();
    REQUIRE(mapper.getActiveVoiceCount() == 0);
    REQUIRE(mapper.getAvailableMpeChannelCount() == 15);
}

TEST_CASE("Stress Test: Editor Lifecycle", "[stress]")
{
    juce::ScopedJuceInitialiser_GUI juceInit; // Required for GUI components in headless tests

    NeoHarpAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    for (int i = 0; i < 50; ++i)
    {
        // 1. Rapidly create and destroy the editor
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        REQUIRE(editor != nullptr);

        // 2. Simulate saving/loading state while UI exists
        juce::MemoryBlock stateData;
        processor.getStateInformation(stateData);
        processor.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));

        editor.reset(); // Destroy Editor
    }

    // Verify it survived
    REQUIRE(processor.getCanonicalContext().scale.numIntervals == 7); // Default
}
