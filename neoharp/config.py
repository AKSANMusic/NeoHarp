# neoharp/config.py

# Zone Boundaries
LH_ZONE = range(21, 60)     # A0–B4: harmonic context / bass
RH_ZONE = range(60, 109)    # C4–C8: remapped performance keys

# Channel Assignments (0-indexed)
LH_CH = 0
RH_CH = 1

# Right-hand key -> scale degree mapping
# Key index 0 = Root, 1 = 2nd, 2 = 3rd, etc., wrapping at octave
RH_KEY_TO_DEGREE = [0, 1, 2, 3, 4, 5, 6]

# Pitch bend range in cents (default is +/- 2 semitones, but we're going with 200 cents as requested in the doc)
PB_RANGE_CENTS = 200.0

# MPE configuration
MPE_CHANNEL_POOL = list(range(1, 15)) # channels 1-14 (0-indexed)
