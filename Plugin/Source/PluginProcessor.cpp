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
    mapper.reset();

    // Set a default C Major Scale to start
    HarmonicContext defaultContext;
    defaultContext.scale.rootMidi = 60;
    defaultContext.scale.numIntervals = 7;
    defaultContext.scale.intervalsCents = { 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f };
    pushNewHarmonicContext(defaultContext);
}

void NeoHarpAudioProcessor::releaseResources()
{
    mapper.reset();
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
    processedBuffer.clear();

    // 1. Lock-free pop of any new state changes from UI. Drain the FIFO completely.
    auto numToRead = contextFifo.getNumReady();
    if (numToRead > 0)
    {
        int start1, size1, start2, size2;
        contextFifo.prepareToRead (numToRead, start1, size1, start2, size2);

        bool contextChanged = false;

        // Apply all events in block 1
        for (int i = 0; i < size1; ++i)
        {
            currentContext = contextBuffer[(start1 + i) % 16];
            contextChanged = true;
        }

        // Apply all events in block 2
        for (int i = 0; i < size2; ++i)
        {
            currentContext = contextBuffer[(start2 + i) % 16];
            contextChanged = true;
        }

        contextFifo.finishedRead (size1 + size2);

        if (contextChanged)
        {
            // 2. Immediately apply context at sample 0 to handle hanging notes safely
            mapper.applyNewContext(currentContext, processedBuffer, 0);
        }
    }

    // 3. Process incoming MIDI
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        mapper.processMessage(message, metadata.samplePosition, processedBuffer);
    }

    // 4. Send our modified buffer to the host
    midiMessages.swapWith(processedBuffer);
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
