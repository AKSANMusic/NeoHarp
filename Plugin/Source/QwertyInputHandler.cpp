#include "QwertyInputHandler.h"

QwertyInputHandler::QwertyInputHandler(juce::MidiMessageCollector& collector)
    : midiCollector(collector)
{
    reset();
}

void QwertyInputHandler::reset()
{
    activeKeys.clear();
    keyToMidiNote.clear();
    // Note: To be perfectly safe, we should probably emit "All Notes Off" or empty the collector here if needed.
}

bool QwertyInputHandler::isKeyCurrentlyDown(int keyCode) const
{
    return juce::KeyPress::isKeyCurrentlyDown(keyCode);
}

double QwertyInputHandler::getMessageTime() const
{
    return juce::Time::getMillisecondCounterHiRes() * 0.001;
}

void QwertyInputHandler::pushCommand(QwertyCommand::Type type, int val)
{
    int start1, size1, start2, size2;
    commandFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0)
    {
        commandBuffer[start1].type = type;
        commandBuffer[start1].value = val;
    }
    commandFifo.finishedWrite(size1 + size2);
}

QwertyCommand QwertyInputHandler::popCommand()
{
    QwertyCommand cmd;
    int start1, size1, start2, size2;
    commandFifo.prepareToRead(1, start1, size1, start2, size2);
    if (size1 > 0)
        cmd = commandBuffer[start1];
    commandFifo.finishedRead(size1 + size2);
    return cmd;
}

bool QwertyInputHandler::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    int keyCode = key.getKeyCode();

    // Suppress auto-repeat
    if (activeKeys[keyCode]) return true;

    activeKeys[keyCode] = true;

    // Check Navigation / Global Modifiers first
    if (keyCode == juce::KeyPress::spaceKey)
    {
        chordLatchEnabled = !chordLatchEnabled;

        // If latch was just disabled, flush any LH keys that are no longer physically held down
        if (!chordLatchEnabled)
        {
            for (auto& pair : activeKeys)
            {
                int k = pair.first;
                int c = (k >= 0 && k < 128) ? std::toupper(k) : k;
                bool isLhRoot = (c == 'A' || c == 'S' || c == 'D' || c == 'F' || c == 'G' || c == 'Q' || c == 'W' || c == 'Z' || c == 'X');

                if (isLhRoot && !isKeyCurrentlyDown(k))
                {
                    pair.second = false; // Mark as no longer active
                    sendNoteOff(k, 1);
                }
            }
        }

        pushCommand(QwertyCommand::Type::ToggleLatch, chordLatchEnabled ? 1 : 0);
        return true;
    }
    if (keyCode == juce::KeyPress::leftKey)
    {
        currentOctaveOffset = juce::jmax(-3, currentOctaveOffset - 1);
        pushCommand(QwertyCommand::Type::OffsetOctave, currentOctaveOffset);
        return true;
    }
    if (keyCode == juce::KeyPress::rightKey)
    {
        currentOctaveOffset = juce::jmin(3, currentOctaveOffset + 1);
        pushCommand(QwertyCommand::Type::OffsetOctave, currentOctaveOffset);
        return true;
    }
    if (keyCode == juce::KeyPress::upKey)
    {
        currentVelocity = juce::jmin(127, currentVelocity + 10);
        pushCommand(QwertyCommand::Type::OffsetVelocity, currentVelocity);
        return true;
    }
    if (keyCode == juce::KeyPress::downKey)
    {
        currentVelocity = juce::jmax(1, currentVelocity - 10);
        pushCommand(QwertyCommand::Type::OffsetVelocity, currentVelocity);
        return true;
    }

    // Pass to sub-handlers
    handleLeftHand(keyCode);
    handleRightHand(keyCode);

    return true; // We consumed the key
}

bool QwertyInputHandler::keyStateChanged (bool, juce::Component*)
{
    // juce::KeyPress doesn't reliably tell us *which* key was released in this callback.
    // We must poll our active map against the OS state.
    for (auto& pair : activeKeys)
    {
        int keyCode = pair.first;
        bool& isDown = pair.second;

        if (isDown)
        {
            if (!isKeyCurrentlyDown(keyCode))
            {
                isDown = false;

                // If it's a left hand key and latch is enabled, we DO NOT send a NoteOff.
                int c = (keyCode >= 0 && keyCode < 128) ? std::toupper(keyCode) : keyCode;
                bool isLhRoot = (c == 'A' || c == 'S' || c == 'D' || c == 'F' || c == 'G' || c == 'Q' || c == 'W' || c == 'Z' || c == 'X');

                if (isLhRoot && chordLatchEnabled)
                {
                    // Do not send NoteOff, keep the chord latched
                    continue;
                }

                sendNoteOff(keyCode, 1);
            }
        }
    }
    return true;
}

void QwertyInputHandler::handleLeftHand(int keyCode)
{
    int c = (keyCode >= 0 && keyCode < 128) ? std::toupper(keyCode) : keyCode;

    // LH Roots mapped to MIDI notes in the LH Zone (21-60).
    // Let's use C2 (36) as the base.
    int lhBase = 36;
    int rootNote = -1;

    // Home Row: Diatonic scale roots (I, II, III, IV, V)
    if      (c == 'A') rootNote = lhBase;     // I (C2)
    else if (c == 'S') rootNote = lhBase + 2; // II (D2)
    else if (c == 'D') rootNote = lhBase + 4; // III (E2)
    else if (c == 'F') rootNote = lhBase + 5; // IV (F2)
    else if (c == 'G') rootNote = lhBase + 7; // V (G2)

    // Top Row: VI, VII
    else if (c == 'Q') rootNote = lhBase + 9; // VI (A2)
    else if (c == 'W') rootNote = lhBase + 11;// VII (B2)

    // Bottom Row: Roots/Fifths
    else if (c == 'Z') rootNote = lhBase - 12; // Lower Root (C1)
    else if (c == 'X') rootNote = lhBase - 5;  // Lower Fifth (G1)

    if (rootNote != -1)
    {
        sendNoteOn(keyCode, 1, rootNote);
        return;
    }

    // Modifiers E, R, T
    if (c == 'E' || c == 'R' || c == 'T')
    {
        // For PoC: Treat these as secondary harmonic context selectors.
        // Send a command to UI to change scale
        int scaleIdx = (c == 'E') ? 1 : ((c == 'R') ? 2 : 3);
        pushCommand(QwertyCommand::Type::SetScale, scaleIdx);
        return;
    }

    // Number Row: Scale Presets
    if (c >= '1' && c <= '5')
    {
        pushCommand(QwertyCommand::Type::SetScale, c - '1');
        return;
    }

    // Bottom Row: Strum Triggers
    // In our PluginProcessor, C1 (24) = DownStrum. We'll map C1, C#1, D1
    if      (c == 'C') sendNoteOn(keyCode, 1, 24); // Down Strum
    else if (c == 'V') sendNoteOn(keyCode, 1, 25); // Up Strum
    else if (c == 'B') sendNoteOn(keyCode, 1, 26); // Mute
}

void QwertyInputHandler::handleRightHand(int keyCode)
{
    int c = (keyCode >= 0 && keyCode < 128) ? std::toupper(keyCode) : keyCode;

    // RH Performance Keys map to the RH Zone (60-109). Base is C4 (60).
    int rhBase = 60 + (currentOctaveOffset * 12);
    int degreeNote = -1;

    // Lower Octave Strings: N, M, ,, ., /
    if      (c == 'N') degreeNote = rhBase - 5;
    else if (c == 'M') degreeNote = rhBase - 3;
    else if (c == ',' || c == 44) degreeNote = rhBase - 1;
    else if (c == '.' || c == 46) degreeNote = rhBase + 0;
    else if (c == '/' || c == 47) degreeNote = rhBase + 2;

    // Middle Octave Strings: H, J, K, L, ;, '
    else if (c == 'H') degreeNote = rhBase + 4;
    else if (c == 'J') degreeNote = rhBase + 5;
    else if (c == 'K') degreeNote = rhBase + 7;
    else if (c == 'L') degreeNote = rhBase + 9;
    else if (c == ';' || c == 59) degreeNote = rhBase + 11;
    // Catch2 passes uppercase for single quote (or we handle char code)
    else if (c == '\'' || c == 39) degreeNote = rhBase + 12;

    // Upper Octave Strings: Y, U, I, O, P, [, ], \
    else if (c == 'Y') degreeNote = rhBase + 12;
    else if (c == 'U') degreeNote = rhBase + 14;
    else if (c == 'I') degreeNote = rhBase + 16;
    else if (c == 'O') degreeNote = rhBase + 17;
    else if (c == 'P') degreeNote = rhBase + 19;
    else if (c == '[' || c == 91) degreeNote = rhBase + 21;
    else if (c == ']' || c == 93) degreeNote = rhBase + 23;
    else if (c == '\\' || c == 92) degreeNote = rhBase + 24;

    if (degreeNote != -1)
    {
        // Clamp to RH Zone bounds (assuming 48 to 109 based on plan)
        degreeNote = juce::jlimit(48, 109, degreeNote);
        sendNoteOn(keyCode, 1, degreeNote);
    }
}

void QwertyInputHandler::sendNoteOn(int keyCode, int midiChannel, int midiNote)
{
    keyToMidiNote[keyCode] = midiNote;
    midiCollector.addMessageToQueue(juce::MidiMessage::noteOn(midiChannel, midiNote, (juce::uint8)currentVelocity));
}

void QwertyInputHandler::sendNoteOff(int keyCode, int midiChannel)
{
    if (keyToMidiNote.find(keyCode) != keyToMidiNote.end())
    {
        int midiNote = keyToMidiNote[keyCode];
        if (midiNote != -1)
        {
            midiCollector.addMessageToQueue(juce::MidiMessage::noteOff(midiChannel, midiNote));
            keyToMidiNote[keyCode] = -1;
        }
    }
}
