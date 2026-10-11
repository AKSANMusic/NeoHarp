import pytest
from neoharp.scales import Scale, SCALES

def test_major_scale_mapping():
    # C Major
    scale = Scale(root_midi=60, intervals_cents=SCALES["major"])

    # Root (degree 0) -> C4 (60), no pitch bend
    note, pb = scale.degree_to_pitch_event(0, 0)
    assert note == 60
    assert pb == 0

    # Octave (degree 7) -> C5 (72), no pitch bend
    note, pb = scale.degree_to_pitch_event(7, 0)
    assert note == 72
    assert pb == 0

    # 3rd (degree 2) -> E4 (64), no pitch bend
    note, pb = scale.degree_to_pitch_event(2, 0)
    assert note == 64
    assert pb == 0

def test_quarter_chromatic_mapping():
    # Quarter tones starting from C4
    scale = Scale(root_midi=60, intervals_cents=SCALES["quarter_chromatic"], pb_range_cents=200.0)

    # Degree 1 (50 cents) -> C4 (60) + 50 cents pitch bend
    # 50 cents out of 200 cents range is 25%, 0.25 * 8192 = 2048
    note, pb = scale.degree_to_pitch_event(1, 0)
    assert note == 60
    assert pb == 2048

    # Degree 3 (150 cents) -> D4 (62) - 50 cents pitch bend
    # 60 * 100 + 150 = 6150. round(61.5) = 62. residual = -50.
    note, pb = scale.degree_to_pitch_event(3, 0)
    assert note == 62
    assert pb == -2048

    # Degree 23 (1150 cents) -> C5 (72) - 50 cents pitch bend
    # 60 * 100 + 1150 = 7150. round(71.5) = 72. residual = -50.
    note, pb = scale.degree_to_pitch_event(23, 0)
    assert note == 72
    assert pb == -2048

def test_maqam_rast_mapping():
    # Maqam Rast: [0, 200, 350, 500, 700, 900, 1050]
    scale = Scale(root_midi=60, intervals_cents=SCALES["maqam_rast"], pb_range_cents=200.0)

    # Degree 2 (350 cents) -> E4 (64) - 50 cents pitch bend (because 6000 + 350 = 6350, closest midi is 6400, so residual is -50)
    # -50 cents out of 200 cents range is -25%, -0.25 * 8192 = -2048
    note, pb = scale.degree_to_pitch_event(2, 0)
    assert note == 64
    assert pb == -2048

    # Degree 6 (1050 cents) -> Bb4 (70) + 50 cents or B4 (71) - 50 cents.
    # 60 + 10.5 = 70.5. Python's round(70.5) is 70 (rounds to nearest even).
    # residual is 50.
    note, pb = scale.degree_to_pitch_event(6, 0)
    assert note == 70
    assert pb == 2048
