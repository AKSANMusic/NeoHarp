#include "StrumEngine.h"
#include <algorithm>

void StrumEngine::reset()
{
    for (auto& sn : scheduledQueue)
        sn.isActive = false;
}

void StrumEngine::prepareToPlay(double sampleRate)
{
    if (sampleRate > 0)
        currentSampleRate = sampleRate;
}

void StrumEngine::scheduleStrum(const int* notes, size_t count, double intervalMs, bool downStrum, int velocityStart, int velocityDecay, double currentBpm, double musicalNoteDivision)
{
    if (count == 0) return;

    // Copy notes into temporary array to sort
    std::array<int, maxScheduledNotes> sortedNotes;
    size_t actualCount = std::min(count, maxScheduledNotes);

    for (size_t i = 0; i < actualCount; ++i)
        sortedNotes[i] = notes[i];

    // Sort: 'down' strum implies highest pitch first
    if (downStrum)
    {
        std::sort(sortedNotes.begin(), sortedNotes.begin() + actualCount, std::greater<int>());
    }
    else
    {
        std::sort(sortedNotes.begin(), sortedNotes.begin() + actualCount, std::less<int>());
    }

    // Determine timing
    double finalIntervalMs = intervalMs;
    if (finalIntervalMs <= 0.0)
    {
        // Fall back to BPM tracking. musicalNoteDivision = 0.25 (1/16th note), 1.0 (quarter note), etc.
        // 1 quarter note in ms = (60,000 / BPM).
        double quarterNoteMs = 60000.0 / currentBpm;
        finalIntervalMs = quarterNoteMs * musicalNoteDivision;
    }

    double samplesPerMs = currentSampleRate / 1000.0;

    // Clear the queue before scheduling a new strum
    reset();

    for (size_t i = 0; i < actualCount; ++i)
    {
        scheduledQueue[i].isActive = true;
        scheduledQueue[i].inputNote = sortedNotes[i];

        int vel = velocityStart - (static_cast<int>(i) * velocityDecay);
        scheduledQueue[i].velocity = juce::jlimit(1, 127, vel);

        // Offset by intervalMs * i
        scheduledQueue[i].samplesRemaining = static_cast<long long>(std::round(finalIntervalMs * i * samplesPerMs));
    }
}

void StrumEngine::processBlock(juce::MidiBuffer& outputBuffer, int numSamples)
{
    for (auto& sn : scheduledQueue)
    {
        if (sn.isActive)
        {
            if (sn.samplesRemaining < numSamples)
            {
                // Note triggers inside this block
                int offset = std::max(0, static_cast<int>(sn.samplesRemaining));

                // We encode this as a NoteOn. The mapper will later consume this.
                // NOTE: We don't map pitch here, we just emit the input note.
                // The PluginProcessor will route this output into PitchMapper.
                outputBuffer.addEvent(juce::MidiMessage::noteOn(1, sn.inputNote, (juce::uint8)sn.velocity), offset);

                sn.isActive = false;
            }
            else
            {
                // Note triggers in a future block
                sn.samplesRemaining -= numSamples;
            }
        }
    }
}
