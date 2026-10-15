#include "ScaleBuilderComponent.h"

ScaleBuilderComponent::ScaleBuilderComponent(NeoHarpAudioProcessor& p)
    : audioProcessor(p)
{
    // Initialize working context from the canonical state
    workingContext = audioProcessor.getCanonicalContext();
}

void ScaleBuilderComponent::paint(juce::Graphics& g)
{
    // Get latest state (in case it was updated via a DAW preset load)
    if (draggingDegree == -1)
        workingContext = audioProcessor.getCanonicalContext();

    gridBounds = getLocalBounds().reduced(20, 20);

    // Draw background
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(gridBounds);

    // Draw horizontal reference lines (0, 100, 200 ... 1200 cents)
    g.setColour(juce::Colours::darkgrey);
    for (int i = 0; i <= 12; ++i)
    {
        int y = centsToY(i * 100.0f);
        g.drawLine(gridBounds.getX(), y, gridBounds.getRight(), y);

        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(10.0f);
        g.drawText(juce::String(i * 100) + "c",
                   gridBounds.getX() - 30, y - 5, 25, 10, juce::Justification::right, false);
        g.setColour(juce::Colours::darkgrey);
    }

    // Draw intervals as vertical bars with nodes
    size_t numDegrees = workingContext.scale.numIntervals;
    if (numDegrees == 0) return;

    int colWidth = gridBounds.getWidth() / static_cast<int>(numDegrees);

    for (size_t i = 0; i < numDegrees; ++i)
    {
        float cents = workingContext.scale.intervalsCents[i];
        int x = gridBounds.getX() + (i * colWidth) + (colWidth / 2);
        int y = centsToY(cents);

        // Draw stem
        g.setColour(juce::Colours::grey);
        g.drawLine(x, gridBounds.getBottom(), x, y, 2.0f);

        // Draw node
        if (static_cast<int>(i) == draggingDegree)
            g.setColour(juce::Colours::red);
        else
            g.setColour(juce::Colours::cyan);

        g.fillEllipse(x - 6, y - 6, 12, 12);

        // Draw current value
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(juce::String(static_cast<int>(cents)) + "c",
                   x - 15, y - 20, 30, 15, juce::Justification::centred, false);
    }
}

void ScaleBuilderComponent::mouseDown(const juce::MouseEvent& event)
{
    size_t numDegrees = workingContext.scale.numIntervals;
    if (numDegrees == 0) return;

    int colWidth = gridBounds.getWidth() / static_cast<int>(numDegrees);

    // Find which degree was clicked
    for (size_t i = 0; i < numDegrees; ++i)
    {
        int x = gridBounds.getX() + (i * colWidth) + (colWidth / 2);
        int y = centsToY(workingContext.scale.intervalsCents[i]);

        juce::Rectangle<int> nodeHitbox(x - 10, y - 10, 20, 20);

        if (nodeHitbox.contains(event.getPosition()))
        {
            draggingDegree = static_cast<int>(i);
            // Root note (degree 0) and Octave (if any) shouldn't be draggable usually,
            // but for microtonal experimentation we'll allow all but 0 to move.
            if (draggingDegree == 0) draggingDegree = -1;
            break;
        }
    }
}

void ScaleBuilderComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (draggingDegree == -1) return;

    float newCents = yToCents(event.y);
    newCents = juce::jlimit(0.0f, 1200.0f, newCents);

    // Optional: add shift-click for fine tuning, normal for 50-cent snap
    if (!event.mods.isShiftDown())
    {
        newCents = std::round(newCents / 50.0f) * 50.0f;
    }

    workingContext.scale.intervalsCents[draggingDegree] = newCents;

    // Constrain to not cross adjacent degrees
    if (draggingDegree > 0)
    {
        float prev = workingContext.scale.intervalsCents[draggingDegree - 1];
        if (newCents < prev) workingContext.scale.intervalsCents[draggingDegree] = prev;
    }
    if (draggingDegree < static_cast<int>(workingContext.scale.numIntervals) - 1)
    {
        float next = workingContext.scale.intervalsCents[draggingDegree + 1];
        if (newCents > next) workingContext.scale.intervalsCents[draggingDegree] = next;
    }

    // Push update to processor
    audioProcessor.setHarmonicContext(workingContext);

    repaint();
}

void ScaleBuilderComponent::mouseUp(const juce::MouseEvent&)
{
    if (draggingDegree != -1)
    {
        // Push final state
        audioProcessor.setHarmonicContext(workingContext);
        draggingDegree = -1;
        repaint();
    }
}

float ScaleBuilderComponent::yToCents(int y) const
{
    float proportion = 1.0f - (static_cast<float>(y - gridBounds.getY()) / gridBounds.getHeight());
    return proportion * 1200.0f;
}

int ScaleBuilderComponent::centsToY(float cents) const
{
    float proportion = cents / 1200.0f;
    return gridBounds.getY() + static_cast<int>((1.0f - proportion) * gridBounds.getHeight());
}
