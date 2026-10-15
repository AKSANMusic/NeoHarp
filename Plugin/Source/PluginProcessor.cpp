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
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Cache the parameter pointer for RT-safe access in processBlock
    strumSpeedParam = apvts.getRawParameterValue("STRUM_SPEED");
}

NeoHarpAudioProcessor::~NeoHarpAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout NeoHarpAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Core parameters based on requirements
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "STRUM_SPEED", 1 },
        "Strum Speed",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        20.0f)); // intervalMs default 20ms

    return { params.begin(), params.end() };
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
    strumEngine.prepareToPlay(sampleRate);
    strumEngine.reset();

    // Ensure the audio thread starts with the canonical context
    // If running in a DAW, setStateInformation will have already populated this
    // from the project save file before prepareToPlay is called.
    if (canonicalContext.scale.numIntervals == 0)
    {
        HarmonicContext defaultContext;
        defaultContext.scale.rootMidi = 60;
        defaultContext.scale.numIntervals = 7;
        defaultContext.scale.intervalsCents = { 0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f };
        setHarmonicContext(defaultContext);
    }
    else
    {
        // Force push the restored canonical context to the audio thread
        setHarmonicContext(canonicalContext);
    }
}

void NeoHarpAudioProcessor::releaseResources()
{
    mapper.reset();
    strumEngine.reset();
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
    strumBuffer.clear();
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
            currentContext = contextBuffer[(start1 + i) % 64];
            contextChanged = true;
        }

        // Apply all events in block 2
        for (int i = 0; i < size2; ++i)
        {
            currentContext = contextBuffer[(start2 + i) % 64];
            contextChanged = true;
        }

        contextFifo.finishedRead (size1 + size2);

        if (contextChanged)
        {
            // 2. Immediately apply context at sample 0 to handle hanging notes safely
            mapper.applyNewContext(currentContext, processedBuffer, 0);
        }
    }

    // 3. Process incoming MIDI (from host)
    // Here we can intercept chords intended to be strummed.
    // For now, we will pass them directly to the mapper,
    // but we can trigger the strum engine via CC or UI.
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        // As a PoC hook: If we receive a C1 (note 24) on LH, schedule a strum!
        if (message.isNoteOn() && message.getNoteNumber() == 24)
        {
            float speedMs = strumSpeedParam->load();

            // Get BPM from host for sync
            double bpm = 120.0;
            if (auto* playHead = getPlayHead())
            {
                if (auto position = playHead->getPosition())
                {
                    if (auto hostBpm = position->getBpm())
                    {
                        bpm = *hostBpm;
                    }
                }
            }

            int chord[6] = { 60, 62, 64, 65, 67, 69 }; // Some RH keys

            // If speedMs is 0.0, strumEngine will use BPM-synced 1/16th notes instead of arbitrary milliseconds
            strumEngine.scheduleStrum(chord, 6, speedMs, true, 100, 5, bpm, 0.25);
        }
        else
        {
            mapper.processMessage(message, metadata.samplePosition, processedBuffer);
        }
    }

    // 4. Process Strum Engine (injects scheduled note-ons into strumBuffer)
    strumEngine.processBlock(strumBuffer, buffer.getNumSamples());

    // 5. Route strum output back through PitchMapper
    for (const auto metadata : strumBuffer)
    {
        auto message = metadata.getMessage();
        mapper.processMessage(message, metadata.samplePosition, processedBuffer);
    }

    // 6. Send our modified buffer to the host
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
    // Save APVTS state
    auto state = apvts.copyState();

    // Save HarmonicContext to ValueTree
    juce::ValueTree contextTree ("HarmonicContext");
    contextTree.setProperty ("rootMidi", canonicalContext.scale.rootMidi, nullptr);
    contextTree.setProperty ("numIntervals", static_cast<int>(canonicalContext.scale.numIntervals), nullptr);
    contextTree.setProperty ("transitionMode", static_cast<int>(canonicalContext.transitionMode), nullptr);

    juce::ValueTree intervalsTree ("Intervals");
    for (size_t i = 0; i < canonicalContext.scale.numIntervals; ++i)
    {
        juce::ValueTree interval ("Interval");
        interval.setProperty ("cents", canonicalContext.scale.intervalsCents[i], nullptr);
        intervalsTree.appendChild(interval, nullptr);
    }
    contextTree.appendChild(intervalsTree, nullptr);

    // Remove old context if it exists, append new
    state.removeChild(state.getChildWithName("HarmonicContext"), nullptr);
    state.appendChild(contextTree, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NeoHarpAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore APVTS state
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml (*xmlState);
            apvts.replaceState (newState);

            // Restore HarmonicContext
            auto contextTree = newState.getChildWithName("HarmonicContext");
            if (contextTree.isValid())
            {
                HarmonicContext restoredContext;
                restoredContext.scale.rootMidi = contextTree.getProperty("rootMidi", 60);
                restoredContext.scale.numIntervals = static_cast<size_t>(static_cast<int>(contextTree.getProperty("numIntervals", 0)));
                restoredContext.transitionMode = static_cast<TransitionMode>(static_cast<int>(contextTree.getProperty("transitionMode", 0)));

                auto intervalsTree = contextTree.getChildWithName("Intervals");
                if (intervalsTree.isValid())
                {
                    int i = 0;
                    for (auto interval : intervalsTree)
                    {
                        if (i < 24)
                            restoredContext.scale.intervalsCents[i++] = interval.getProperty("cents", 0.0f);
                    }
                }

                // Directly set the canonical context and push to FIFO so audio thread picks it up
                setHarmonicContext(restoredContext);
            }
        }
    }
}

//==============================================================================
void NeoHarpAudioProcessor::setHarmonicContext (const HarmonicContext& newContext)
{
    // 1. Update the canonical copy on the message thread
    canonicalContext = newContext;

    // 2. Push to lock-free FIFO for the audio thread
    int start1, size1, start2, size2;
    contextFifo.prepareToWrite (1, start1, size1, start2, size2);

    if (size1 > 0)
        contextBuffer[start1] = newContext;

    contextFifo.finishedWrite (size1 + size2);

    // (Note: APVTS serialization happens automatically in getStateInformation when DAW saves)
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeoHarpAudioProcessor();
}
