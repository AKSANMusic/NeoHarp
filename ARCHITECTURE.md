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
* **`CMakeLists.txt`**: Uses CMake `FetchContent` to download JUCE automatically during the configure phase. It is configured to build `NeoHarp` as a VST3 plugin and Standalone app, marked correctly as a MIDI effect.
* **`Source/PluginProcessor.h/cpp`**: The core audio processing module, integrating the `PitchMapper` logic into `processBlock`.
* **`Source/PluginEditor.h/cpp`**: The scaffolding for the custom UI.
* **`Source/HarmonicContext.h`**: The critical real-time safe data structures.
* **`Source/PitchMapper.h/cpp`**: The decoupled C++ core engine responsible for microtonal MPE routing and voice stealing.

### 2.2 Lock-Free State Management & Real-Time Safety
The most important architectural constraint for a C++ audio plugin is avoiding memory allocation, locks, or blocking operations on the audio thread.

1. **State Transfer:** The `HarmonicContext.h` defines the `Scale` and `TransitionMode`. `std::vector` was explicitly avoided in the `Scale` struct. Instead, a fixed-size `std::array<float, 24>` is used. The `PluginProcessor` owns a `juce::AbstractFifo` to pass contexts from the UI to the audio thread entirely lock-free.
2. **Audio Processing (`processBlock`):** Inside `processBlock()`, the engine does a lock-free check (`getNumReady()`) and pops the new state. If a state change occurs, it asks the `PitchMapper` to immediately issue Note-Offs for hanging notes *before* parsing the incoming MIDI buffers.
3. **PitchMapper Engine:** The `PitchMapper` tracks active voices and the MPE channel pool (channels 2-16) exclusively using pre-sized `std::array` containers. `std::map` and `new` keywords are completely omitted to ensure zero heap allocations during live performance.

### 2.3 Next Steps (C++ Engine)
With the remapping core integrated safely, the remaining Phase 3 milestones are:
1. **Strum Engine:** Utilizing JUCE's `MidiBuffer` to achieve sample-accurate output scheduling for guitar-like strums (replacing Python's `time.sleep` emulator).
2. **Plugin GUI:** Building out the `PluginEditor` UI components (sliders, grids) to construct and push `HarmonicContext` changes to the FIFO.
3. **Preset Management:** Saving/Loading XML or JSON representations of scales so the plugin state can be recalled by the host DAW.