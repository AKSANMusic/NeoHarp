#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NeoHarpAudioProcessor::NeoHarpAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

NeoHarpAudioProcessor::~NeoHarpAudioProcessor()
{
}

//==============================================================================
const juce::String NeoHarpAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NeoHarpAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NeoHarpAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NeoHarpAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NeoHarpAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NeoHarpAudioProcessor::getNumPrograms()
{
    return 1;
}

int NeoHarpAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NeoHarpAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NeoHarpAudioProcessor::getProgramName (int index)
{
    return {};
}

void NeoHarpAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NeoHarpAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Initialize context
    contextFifo.reset();
}

void NeoHarpAudioProcessor::releaseResources()
{
}

bool NeoHarpAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void NeoHarpAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    // 1. Lock-free pop of any new state changes from UI
    auto numToRead = contextFifo.getNumReady();
    if (numToRead > 0)
    {
        int start1, size1, start2, size2;
        contextFifo.prepareToRead (1, start1, size1, start2, size2);

        if (size1 > 0)
            currentContext = contextBuffer[start1];

        contextFifo.finishedRead (size1 + size2);
    }

    // 2. Process MIDI
    // In Phase 3, this is where MidiZoneRouter and PitchMapper will operate.
    // For now, we just pass MIDI through.

    // juce::MidiBuffer processedBuffer;
    // for (const auto metadata : midiMessages)
    // {
    //     auto message = metadata.getMessage();
    //     // Modify message...
    //     // processedBuffer.addEvent(message, metadata.samplePosition);
    // }
    // midiMessages.swapWith(processedBuffer);
}

//==============================================================================
bool NeoHarpAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* NeoHarpAudioProcessor::createEditor()
{
    return new NeoHarpAudioProcessorEditor (*this);
}

//==============================================================================
void NeoHarpAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

void NeoHarpAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

//==============================================================================
void NeoHarpAudioProcessor::pushNewHarmonicContext (const HarmonicContext& newContext)
{
    int start1, size1, start2, size2;
    contextFifo.prepareToWrite (1, start1, size1, start2, size2);

    if (size1 > 0)
        contextBuffer[start1] = newContext;

    contextFifo.finishedWrite (size1 + size2);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeoHarpAudioProcessor();
}
