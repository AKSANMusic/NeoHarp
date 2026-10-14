#include "PitchMapper.h"

PitchMapper::PitchMapper()
{
    reset();
}

void PitchMapper::reset()
{
    for (auto& voice : activeVoices)
        voice.isActive = false;

    // Initialize MPE channels 2-16 (1-indexed for JUCE)
    for (size_t i = 0; i < 15; ++i)
        mpeChannelPool[i] = static_cast<int>(i + 2);
    numAvailableMpeChannels = 15;
    numActiveVoices = 0;
}

void PitchMapper::setZones(int lhMin, int lhMax, int rhMin, int rhMax)
{
    lhZoneMin = lhMin;
    lhZoneMax = lhMax;
    rhZoneMin = rhMin;
    rhZoneMax = rhMax;
}

void PitchMapper::applyNewContext(const HarmonicContext& newContext, juce::MidiBuffer& outputBuffer, int samplePosition)
{
    // If the transition mode is HARD_CUT or LEGATO_RETRIG, or if the scale changed, we must release old voices.
    // In Python PoC we did this on every scale change unconditionally.
    releaseAllRhVoices(outputBuffer, samplePosition);
    currentContext = newContext;
}

void PitchMapper::processMessage(const juce::MidiMessage& message, int samplePosition, juce::MidiBuffer& outputBuffer)
{
    if (message.isNoteOn())
    {
        int note = message.getNoteNumber();
        int velocity = message.getVelocity();

        if (note >= lhZoneMin && note < lhZoneMax)
        {
            // Update root, but do not change the intervals yet
            // The UI thread usually drives HarmonicContext, but in the PoC LH zone sets root
            // We'll mimic that here to keep the PoC self-contained for tests
            currentContext.scale.rootMidi = note;
            releaseAllRhVoices(outputBuffer, samplePosition);
            outputBuffer.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)velocity), samplePosition);
        }
        else if (note >= rhZoneMin && note < rhZoneMax)
        {
            int outPitch = 0, outPb = 0;
            calculatePitch(note, outPitch, outPb);

            int channel = allocateChannel(note, outputBuffer, samplePosition);

            // Send Pitch bend before note on
            if (outPb != 0)
            {
                // outPb is signed -8192 to 8191. JUCE pitchWheelValue takes 0 to 16383.
                int unsignedPb = outPb + 8192;
                outputBuffer.addEvent(juce::MidiMessage::pitchWheel(channel, unsignedPb), samplePosition);
            }

            outputBuffer.addEvent(juce::MidiMessage::noteOn(channel, outPitch, (juce::uint8)velocity), samplePosition);

            activeVoices[static_cast<size_t>(note)] = { true, note, channel, outPitch, outPb };
        }
    }
    else if (message.isNoteOff())
    {
        int note = message.getNoteNumber();

        if (note >= lhZoneMin && note < lhZoneMax)
        {
            outputBuffer.addEvent(juce::MidiMessage::noteOff(1, note), samplePosition);
        }
        else if (note >= rhZoneMin && note < rhZoneMax)
        {
            auto& voice = activeVoices[static_cast<size_t>(note)];
            if (voice.isActive)
            {
                outputBuffer.addEvent(juce::MidiMessage::noteOff(voice.outChannel, voice.outPitch), samplePosition);
                freeChannel(note);
            }
        }
    }
    else if (message.isController())
    {
        // CC passthrough to LH channel by default
        outputBuffer.addEvent(juce::MidiMessage::controllerEvent(1, message.getControllerNumber(), message.getControllerValue()), samplePosition);
    }
}

void PitchMapper::calculatePitch(int inputNote, int& outPitch, int& outPb)
{
    if (currentContext.scale.numIntervals == 0)
    {
        outPitch = inputNote;
        outPb = 0;
        return;
    }

    // Default right hand key to degree mapping: 0,1,2,3,4,5,6
    int idx = inputNote - rhZoneMin;
    int degree = idx % 7;
    int octave = idx / 7;

    // Safety bounds on degree lookup
    if (static_cast<size_t>(degree) >= currentContext.scale.numIntervals)
        degree = static_cast<int>(currentContext.scale.numIntervals) - 1;

    float absCents = (currentContext.scale.rootMidi * 100.0f)
                   + currentContext.scale.intervalsCents[static_cast<size_t>(degree)]
                   + (octave * 1200.0f);

    outPitch = juce::jlimit(0, 127, static_cast<int>(std::round(absCents / 100.0f)));
    float residual = absCents - (outPitch * 100.0f);

    // pb range 200 cents
    outPb = static_cast<int>((residual / 200.0f) * 8192.0f);
    outPb = juce::jlimit(-8192, 8191, outPb);
}

int PitchMapper::allocateChannel(int inputNote, juce::MidiBuffer& outputBuffer, int samplePosition)
{
    int channel = 2; // Default if pool is empty somehow

    if (numAvailableMpeChannels > 0)
    {
        channel = mpeChannelPool[--numAvailableMpeChannels];
    }
    else if (numActiveVoices > 0)
    {
        // Steal oldest
        int oldestNote = activeVoiceNotes[0];
        auto& stolenVoice = activeVoices[static_cast<size_t>(oldestNote)];

        outputBuffer.addEvent(juce::MidiMessage::noteOff(stolenVoice.outChannel, stolenVoice.outPitch), samplePosition);
        channel = stolenVoice.outChannel;
        stolenVoice.isActive = false;

        // Shift queue down
        for (size_t i = 0; i < numActiveVoices - 1; ++i)
            activeVoiceNotes[i] = activeVoiceNotes[i + 1];
        numActiveVoices--;
    }

    // Add to active queue
    if (numActiveVoices < 15)
        activeVoiceNotes[numActiveVoices++] = inputNote;

    return channel;
}

void PitchMapper::freeChannel(int inputNote)
{
    auto& voice = activeVoices[static_cast<size_t>(inputNote)];
    if (!voice.isActive) return;

    if (numAvailableMpeChannels < 15)
        mpeChannelPool[numAvailableMpeChannels++] = voice.outChannel;

    // Remove from active queue
    for (size_t i = 0; i < numActiveVoices; ++i)
    {
        if (activeVoiceNotes[i] == inputNote)
        {
            for (size_t j = i; j < numActiveVoices - 1; ++j)
                activeVoiceNotes[j] = activeVoiceNotes[j + 1];
            numActiveVoices--;
            break;
        }
    }

    voice.isActive = false;
}

void PitchMapper::releaseAllRhVoices(juce::MidiBuffer& outputBuffer, int samplePosition)
{
    for (size_t i = 0; i < 128; ++i)
    {
        if (activeVoices[i].isActive)
        {
            outputBuffer.addEvent(juce::MidiMessage::noteOff(activeVoices[i].outChannel, activeVoices[i].outPitch), samplePosition);

            if (numAvailableMpeChannels < 15)
                mpeChannelPool[numAvailableMpeChannels++] = activeVoices[i].outChannel;

            activeVoices[i].isActive = false;
        }
    }
    numActiveVoices = 0;
}
