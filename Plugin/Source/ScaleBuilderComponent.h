#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Forward declaration
class NeoHarpAudioProcessor;

class ScaleBuilderComponent : public juce::Component
{
public:
    ScaleBuilderComponent(NeoHarpAudioProcessor& p);
    ~ScaleBuilderComponent() override = default;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    NeoHarpAudioProcessor& audioProcessor;

    // The working copy of the state while dragging
    HarmonicContext workingContext;

    int draggingDegree { -1 };

    // UI drawing bounds
    juce::Rectangle<int> gridBounds;

    // Helper to map y-pixel to cents and vice versa
    float yToCents(int y) const;
    int centsToY(float cents) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleBuilderComponent)
};
