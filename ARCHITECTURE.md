# NeoHarp Architecture & Implementation Report

This document outlines the current state of the NeoHarp repository. It successfully implements Phase 1 (Python Proof-of-Concept) and the core engine for Phase 3 (JUCE C++ Production Plugin) of the initial technical feasibility assessment.

---

## 1. Phase 1: Python Proof-of-Concept (PoC)

The Python implementation serves as a functional reference for the core MIDI remapping logic, scale definitions, and stuck-note prevention systems. It allows for rapid iteration of algorithms before they are ported to real-time audio threads.

### 1.1 Directory Structure (`neoharp/` & `tests/`)
* **`engine.py`**: Contains the core `NeoHarpEngine` class. It manages state across left-hand (LH) harmonic context shifts and right-hand (RH) performance inputs. It correctly implements voice tracking, MPE channel allocation, and voice stealing to guarantee Note-Off events are routed to the old pitches when the harmonic context shifts.
* **`scales.py`**: Defines the `Scale` class and an initial library of microtonal modes (e.g., 12-TET Major, Quarter-tone Chromatic, Maqam Rast). It provides the mathematical abstraction to map scale degrees to MIDI Note + 14-bit Pitch Bend residuals.
* **`config.py`**: Extracts the configurable layout, defining `LH_ZONE` (A0–B4), `RH_ZONE` (C4–C8), Pitch Bend Range (200¢), and the MPE Channel Pool.
* **`strum.py`**: Contains a background thread-based strum scheduling emulator. While not sample-accurate, it proves the musical correctness of offset/velocity ramp sequencing.
* **`main.py`**: The entry point that hooks the engine into the system's MIDI I/O using `python-rtmidi`.
* **`tests/`**: Headless unit tests using `pytest` and `pytest-mock` to rigorously test pitch mapping math and chord-change stuck note edge cases.

### 1.2 Mapping to the Initial Spec
* **Hanging Note Problem Solved**: The `NeoHarpEngine` tracks an `ActiveVoice` dictionary mapped to the *input* note. When the LH zone updates the harmonic context, it correctly pops all active voices and sends a Note-Off using the cached output pitch and channel *before* applying the new scale.
* **Cents over Semitones**: The core scale abstraction defines intervals entirely in cents.
* **Latency Profile**: As expected with Python, this stack is subject to GIL constraints and is only suitable for prototyping and testing, not live performance.

---

## 2. Phase 3: Production Plugin Scaffolding (C++ / JUCE)

To achieve sample-accurate scheduling, zero-allocation real-time safety, and DAW compatibility, the repository includes the scaffolding for a JUCE-based VST3 and Standalone application.

### 2.1 Directory Structure (`Plugin/`)
* **`CMakeLists.txt`**: Uses CMake `FetchContent` to download JUCE and Catch2 automatically during the configure phase. It is configured to build `NeoHarp` as a VST3 plugin, a Standalone app, and an independent `NeoHarpTests` headless executable.
* **`Source/PluginProcessor.h/cpp`**: The core audio processing module. Integrates the `PitchMapper` logic, the `StrumEngine`, and the `AudioProcessorValueTreeState` (APVTS).
* **`Source/PluginEditor.h/cpp`**: The plugin GUI. Implements a 30Hz diagnostic timer to display lock-free status updates and parameter sliders.
* **`Source/HarmonicContext.h`**: The critical real-time safe data structures.
* **`Source/PitchMapper.h/cpp`**: The decoupled C++ core engine responsible for microtonal MPE routing and voice stealing.
* **`Source/StrumEngine.h/cpp`**: The sample-accurate strum sequencer synchronized with host BPM.
* **`Tests/PitchMapperTests.cpp`**: Catch2 test suite replicating the Python PoC edge cases in C++.

### 2.2 Lock-Free State Management & Real-Time Safety
The most important architectural constraint for a C++ audio plugin is avoiding memory allocation, locks, or blocking operations on the audio thread.

1. **State Transfer:** The `HarmonicContext.h` defines the `Scale` and `TransitionMode`. `std::vector` was explicitly avoided in the `Scale` struct. Instead, a fixed-size `std::array<float, 24>` is used. The `PluginProcessor` owns a `juce::AbstractFifo` to pass contexts from the UI to the audio thread entirely lock-free.
2. **Audio Processing (`processBlock`):** Inside `processBlock()`, the engine does a lock-free check (`getNumReady()`) and drains the state queue. If a state change occurs, it asks the `PitchMapper` to immediately issue Note-Offs for hanging notes *before* parsing the incoming MIDI buffers. To avoid allocation inside the audio thread, the secondary `juce::MidiBuffer` is declared as a pre-allocated class member variable rather than a local variable.
3. **PitchMapper Engine:** The `PitchMapper` tracks active voices and the MPE channel pool (channels 2-16) exclusively using pre-sized `std::array` containers. `std::map` and `new` keywords are completely omitted to ensure zero heap allocations during live performance.
4. **Strum Engine:** Triggers sample-accurate note delays by reading `juce::AudioPlayHead` BPM, translating musical divisions into sample counts based on the host sample rate, and buffering into `std::array` to avoid allocations on the sequencer.

### 2.3 Next Steps
With the core remapping engine, headless tests, strum timing, and diagnostic APVTS GUI successfully implemented, the remaining steps for production release are:
1. **Full UI Development:** Building out the `PluginEditor` UI grids to visually construct and drag-and-drop `HarmonicContext` scale degree intervals.
2. **Preset Management:** Saving/Loading XML or JSON representations of scales so the plugin state can be recalled by the host DAW natively via APVTS.
3. **DAW Integration Testing:** Live stress-testing inside hosts like Ableton/Reaper with MPE synths (Surge XT/Vital) at variable buffer sizes.