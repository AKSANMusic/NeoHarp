#pragma once

#include <JuceHeader.h>
#include <array>
#include "HarmonicContext.h"

// For passing commands back to the processor or UI
struct QwertyCommand
{
    enum class Type { None, SetScale, OffsetOctave, OffsetVelocity, ToggleLatch };
    Type type { Type::None };
    int value { 0 };
};

class QwertyInputHandler : public juce::KeyListener
{
public:
    QwertyInputHandler(juce::MidiMessageCollector& collector);
    virtual ~QwertyInputHandler() = default;

    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) override;

    // Command access for UI polling
    QwertyCommand popCommand();

    // Check if Latch is enabled
    bool isLatchEnabled() const { return chordLatchEnabled; }

    // Virtual hook for mocking the OS key-down state in unit tests
    virtual bool isKeyCurrentlyDown(int keyCode) const;

    // Virtual hook to get current time for MIDI timestamping
    virtual double getMessageTime() const;

    // Reset all internal state and send NoteOffs
    void reset();

private:
    juce::MidiMessageCollector& midiCollector;

    // State tracking to prevent OS key auto-repeat
    // We use a fixed array sized to hold common ascii keycodes + special keys.
    // Special keys like right arrow are mapped high in JUCE (e.g. 0x10000 | something).
    // Let's use a std::map for this specific handler to handle any JUCE key code safely.
    // Since this runs on the UI thread (key events) and passes to processor via FIFO,
    // using a std::map here does NOT violate audio thread RT constraints!
    std::map<int, bool> activeKeys;

    // Map of which key generated which MIDI note, so we can reliably send NoteOffs even if octave shifted
    std::map<int, int> keyToMidiNote;

    // Current playing state
    int currentVelocity { 100 };
    int currentOctaveOffset { 0 }; // 0 = default, -1 = down an octave, +1 = up an octave
    bool chordLatchEnabled { false };

    // FIFO for pushing commands (like octave shift or scale select) to the UI/Processor
    juce::AbstractFifo commandFifo { 16 };
    QwertyCommand commandBuffer[16];

    void pushCommand(QwertyCommand::Type type, int val = 0);

    // Mappings
    void handleLeftHand(int keyCode);
    void handleRightHand(int keyCode);
    void handleNavigation(int keyCode);

    void sendNoteOn(int keyCode, int midiChannel, int midiNote);
    void sendNoteOff(int keyCode, int midiChannel);
};
