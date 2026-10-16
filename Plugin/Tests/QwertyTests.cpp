#include <catch2/catch_test_macros.hpp>
#include "QwertyInputHandler.h"

// Mock to manually control the OS key state without a real keyboard
class MockQwertyHandler : public QwertyInputHandler
{
public:
    MockQwertyHandler(juce::MidiMessageCollector& c) : QwertyInputHandler(c) {}

    std::map<int, bool> simulatedOsKeyState;

    bool isKeyCurrentlyDown(int keyCode) const override
    {
        auto it = simulatedOsKeyState.find(keyCode);
        if (it != simulatedOsKeyState.end()) return it->second;
        return false;
    }
};

TEST_CASE("QWERTY: Auto-Repeat Suppression & Note-Off", "[qwerty]")
{
    juce::MidiMessageCollector collector;
    collector.reset(44100.0);
    MockQwertyHandler handler(collector);

    // Simulate pressing 'A' (LH Root C2)
    juce::KeyPress keyA('A');
    handler.simulatedOsKeyState['A'] = true;
    handler.keyPressed(keyA, nullptr);

    // OS Auto-repeats 'A' 3 more times because user is holding it
    handler.keyPressed(keyA, nullptr);
    handler.keyPressed(keyA, nullptr);
    handler.keyPressed(keyA, nullptr);

    juce::MidiBuffer buffer;
    collector.removeNextBlockOfMessages(buffer, 1024);

    // Verify only ONE NoteOn was emitted
    int numEvents = 0;
    for (const auto m : buffer)
    {
        REQUIRE(m.getMessage().isNoteOn());
        REQUIRE(m.getMessage().getNoteNumber() == 36); // Base C2
        numEvents++;
    }
    REQUIRE(numEvents == 1);

    // Simulate releasing 'A'
    handler.simulatedOsKeyState['A'] = false;
    handler.keyStateChanged(false, nullptr);

    buffer.clear();
    collector.removeNextBlockOfMessages(buffer, 1024);

    // Verify NoteOff was emitted
    numEvents = 0;
    for (const auto m : buffer)
    {
        REQUIRE(m.getMessage().isNoteOff());
        REQUIRE(m.getMessage().getNoteNumber() == 36);
        numEvents++;
    }
    REQUIRE(numEvents == 1);
}

TEST_CASE("QWERTY: Latch Mode Logic", "[qwerty]")
{
    juce::MidiMessageCollector collector;
    collector.reset(44100.0);
    MockQwertyHandler handler(collector);

    // 1. Enable Latch mode via Spacebar
    juce::KeyPress keySpace(juce::KeyPress::spaceKey);
    handler.simulatedOsKeyState[juce::KeyPress::spaceKey] = true;
    handler.keyPressed(keySpace, nullptr);
    handler.simulatedOsKeyState[juce::KeyPress::spaceKey] = false;
    handler.keyStateChanged(false, nullptr);

    // Check command emitted
    auto cmd = handler.popCommand();
    REQUIRE(cmd.type == QwertyCommand::Type::ToggleLatch);
    REQUIRE(cmd.value == 1); // 1 = enabled

    // 2. Press 'S' (LH Root D2 = 38)
    juce::KeyPress keyS('S');
    handler.simulatedOsKeyState['S'] = true;
    handler.keyPressed(keyS, nullptr);

    juce::MidiBuffer buffer;
    collector.removeNextBlockOfMessages(buffer, 1024);
    REQUIRE(buffer.getNumEvents() == 1); // NoteOn D2

    // 3. Release 'S'
    handler.simulatedOsKeyState['S'] = false;
    handler.keyStateChanged(false, nullptr);

    buffer.clear();
    collector.removeNextBlockOfMessages(buffer, 1024);

    // 4. Verify NO NoteOff was sent because it was an LH root and Latch is ON
    REQUIRE(buffer.getNumEvents() == 0);
}

TEST_CASE("QWERTY: RH Octave Shift Logic", "[qwerty]")
{
    juce::MidiMessageCollector collector;
    collector.reset(44100.0);
    MockQwertyHandler handler(collector);

    // Press Right Arrow (Octave +1)
    juce::KeyPress keyRight(juce::KeyPress::rightKey);
    handler.simulatedOsKeyState[juce::KeyPress::rightKey] = true;
    handler.keyPressed(keyRight, nullptr);
    handler.simulatedOsKeyState[juce::KeyPress::rightKey] = false;
    handler.keyStateChanged(false, nullptr);

    // Press 'J' (RH String, default C4 + 5 = F4 = 65)
    // With octave +1, it should be F5 = 77
    juce::KeyPress keyJ('J');
    handler.simulatedOsKeyState['J'] = true;
    handler.keyPressed(keyJ, nullptr);

    juce::MidiBuffer buffer;
    collector.removeNextBlockOfMessages(buffer, 1024);

    int numEvents = 0;
    for (const auto m : buffer)
    {
        REQUIRE(m.getMessage().isNoteOn());
        REQUIRE(m.getMessage().getNoteNumber() == 77);
        numEvents++;
    }
    REQUIRE(numEvents == 1);

    // Ensure NoteOff uses the same note number
    handler.simulatedOsKeyState['J'] = false;
    handler.keyStateChanged(false, nullptr);

    buffer.clear();
    collector.removeNextBlockOfMessages(buffer, 1024);

    numEvents = 0;
    for (const auto m : buffer)
    {
        REQUIRE(m.getMessage().isNoteOff());
        REQUIRE(m.getMessage().getNoteNumber() == 77);
        numEvents++;
    }
    REQUIRE(numEvents == 1);
}
