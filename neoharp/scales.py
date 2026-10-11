# neoharp/scales.py

from dataclasses import dataclass

@dataclass
class Scale:
    root_midi: int                    # MIDI note number of the root
    intervals_cents: list[float]      # cent offsets for each scale degree
    pb_range_cents: float = 200.0     # semitone = 100c; ±1 semitone = ±200c range

    def degree_to_pitch_event(self, degree: int, octave_offset: int = 0) -> tuple[int, int]:
        """
        Maps a scale degree + octave offset to (midi_note, pb_14bit).
        Handles degree wrapping and octave extension automatically.
        """
        n_degrees = len(self.intervals_cents)
        oct_extra, deg = divmod(degree, n_degrees)
        abs_cents = (
            self.root_midi * 100.0
            + self.intervals_cents[deg]
            + (octave_offset + oct_extra) * 1200.0
        )
        midi_note = max(0, min(127, round(abs_cents / 100.0)))
        residual = abs_cents - midi_note * 100.0
        pb = int((residual / self.pb_range_cents) * 8192)
        return midi_note, max(-8192, min(8191, pb))


SCALES: dict[str, list[float]] = {
    "major":         [0, 200, 400, 500, 700, 900, 1100],
    "minor_nat":     [0, 200, 300, 500, 700, 800, 1000],
    "maqam_rast":    [0, 200, 350, 500, 700, 900, 1050],   # neutral 3rd & 7th
    "maqam_hijaz":   [0, 100, 400, 500, 700, 800, 1100],   # augmented 2nd
    "dastgah_shur":  [0, 150, 300, 500, 700, 850, 1000],   # quartertone inflections
    "turkish_rast":  [0, 204, 386, 498, 702, 884, 1088],   # just intonation variant
    "quarter_chromatic": [i * 50 for i in range(24)],       # 24-tone equal temperament
}
