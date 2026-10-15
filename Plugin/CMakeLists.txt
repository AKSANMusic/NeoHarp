cmake_minimum_required(VERSION 3.20)
project(NeoHarp VERSION 0.1.0)

# ==============================================================================
# Fetch JUCE
# ==============================================================================
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

# ==============================================================================
# Plugin Configuration
# ==============================================================================
juce_add_plugin(NeoHarp
    PLUGIN_MANUFACTURER_CODE "NHRP"
    PLUGIN_CODE "Nhrp"
    IS_SYNTH FALSE             # This is a MIDI effect, not a synth
    IS_MIDI_EFFECT TRUE        # We process MIDI and output MIDI
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    FORMATS VST3 Standalone    # Build VST3 and Standalone formats
    PRODUCT_NAME "NeoHarp"
)

# ==============================================================================
# Target Sources & Includes
# ==============================================================================
target_sources(NeoHarp PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/HarmonicContext.h
    Source/PitchMapper.cpp
    Source/PitchMapper.h
    Source/StrumEngine.cpp
    Source/StrumEngine.h
    Source/ScaleBuilderComponent.cpp
    Source/ScaleBuilderComponent.h
)

juce_generate_juce_header(NeoHarp)

target_compile_definitions(NeoHarp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(NeoHarp PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_events
    juce::juce_core
)

# ==============================================================================
# Catch2 Tests
# ==============================================================================
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(NeoHarpTests
    Tests/PitchMapperTests.cpp
    Tests/StateTests.cpp
    Source/PitchMapper.cpp
    Source/PluginProcessor.cpp
    Source/StrumEngine.cpp
    Source/PluginEditor.cpp
    Source/ScaleBuilderComponent.cpp
)

target_include_directories(NeoHarpTests PRIVATE Source)

# So PitchMapper can find JuceHeader.h when compiled inside NeoHarpTests
target_include_directories(NeoHarpTests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/NeoHarp_artefacts/JuceLibraryCode)

target_link_libraries(NeoHarpTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_extra
)

target_compile_definitions(NeoHarpTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JucePlugin_Name="NeoHarpTests"
)
