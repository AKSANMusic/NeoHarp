# NeoHarp

NeoHarp is a real-time MIDI remapping engine designed to allow playing chords and scales with dynamic microtonal adjustments and advanced MPE routing.

This repository contains two parallel implementations:
1. **Phase 1: Python Proof-of-Concept (PoC)** - A functional reference implementation for validating the core remapping math and note tracking.
2. **Phase 3: JUCE C++ Production Plugin** - A real-time safe, zero-allocation VST3 and Standalone plugin implementation with sample-accurate strum scheduling.

For a deep dive into the real-time C++ constraints and lock-free state architectures, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Architecture

The NeoHarp engine intercepts incoming MIDI messages, mapping the left-hand input to a harmonic root and scale context, and mapping right-hand inputs to specific scale degrees.

### Zone Diagram

- **Left-Hand (LH) Zone (Default A0–B4, keys 21-60):** Controls the harmonic context. Hitting a note in this zone sets the new root for the scale and sends a bass note to the LH channel.
- **Right-Hand (RH) Zone (Default C4–C8, keys 60-109):** The performance keys. These are mapped to degrees in the current scale, with automatic pitch bend applied for microtonal scales.

## Setup & Testing

### Python PoC

1. **Install Dependencies:**
   Ensure you have Python 3.9+ installed, then install the required packages:
   ```bash
   pip install -r requirements.txt
   ```

2. **Run the Engine:**
   ```bash
   python main.py
   ```

3. **Run the Python Tests:**
   Tests mock the `rtmidi` interface so they run headlessly.
   ```bash
   pytest
   ```

### C++ JUCE Plugin

The C++ Plugin requires CMake (3.20+) and a C++20 compatible compiler. CMake uses `FetchContent` to automatically download JUCE and Catch2—no submodules needed.

1. **Build the Plugin and Tests:**
   ```bash
   mkdir -p Plugin/build && cd Plugin/build
   cmake ..
   make -j$(nproc)
   ```

2. **Run the C++ Tests:**
   The `PitchMapper` is decoupled from the `juce::AudioProcessor` so it can be tested completely headlessly via Catch2.
   ```bash
   cd Plugin/build
   ./NeoHarpTests
   ```

3. **Run the Standalone App / VST3:**
   * The Standalone application will be built to `Plugin/build/NeoHarp_artefacts/Standalone/NeoHarp`.
   * The VST3 will be built to `Plugin/build/NeoHarp_artefacts/VST3/NeoHarp.vst3`.
