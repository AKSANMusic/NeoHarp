# NeoHarp PoC

NeoHarp is a real-time MIDI remapping engine designed to allow playing chords and scales with dynamic microtonal adjustments and advanced routing. This repository contains the Phase 1 Python Proof-of-Concept (PoC).

## Architecture

The NeoHarp engine intercepts incoming MIDI messages, mapping the left-hand input to a harmonic root and scale context, and mapping right-hand inputs to specific scale degrees.

### Zone Diagram

- **Left-Hand (LH) Zone (Default A0–B4, keys 21-60):** Controls the harmonic context. Hitting a note in this zone sets the new root for the scale and sends a bass note to the LH channel.
- **Right-Hand (RH) Zone (Default C4–C8, keys 60-109):** The performance keys. These are mapped to degrees in the current scale, with automatic pitch bend applied for microtonal scales.

## Setup

1. **Install Dependencies:**
   Ensure you have Python 3.9+ installed, then install the required packages:
   ```bash
   pip install -r requirements.txt
   ```

2. **Run the Engine:**
   Run the main script to start listening for MIDI input:
   ```bash
   python main.py
   ```
   You can specify virtual ports if needed. You may need loopMIDI (Windows), IAC Driver (macOS), or JACK (Linux) to route virtual MIDI.

## Testing

Tests are written using `pytest`. They mock the `rtmidi` interface so they can be run completely headlessly without MIDI hardware.

```bash
pytest
```

## Known Limitations (PoC Phase)
- Strum emulation uses `threading.Thread` and `time.sleep()`. This validates the musical sequencing but does not guarantee sample-accurate scheduling. This limitation is addressed in the Phase 2/3 implementations (REAPER JSFX / JUCE).
