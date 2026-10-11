import pytest
from neoharp.engine import NeoHarpEngine
from neoharp import config

@pytest.fixture
def engine(mocker):
    engine = NeoHarpEngine(scale_name="major", root=60, mpe=False)
    engine.midi_out = mocker.MagicMock()
    return engine

@pytest.fixture
def mpe_engine(mocker):
    engine = NeoHarpEngine(scale_name="major", root=60, mpe=True)
    engine.midi_out = mocker.MagicMock()
    return engine

def test_note_on_off_basic(engine):
    # Play C4 (root of C major) on RH. Sending rtmidi-style tuple.
    engine.process_message(([0x90, 60, 100], 0.0))

    # Check that note on was sent on RH channel
    # Degree 0 of C major -> note 60, pb 0
    engine.midi_out.send_message.assert_called_with([0x90 | config.RH_CH, 60, 100])

    # Note off
    engine.process_message(([0x80, 60, 0], 0.1))
    engine.midi_out.send_message.assert_called_with([0x80 | config.RH_CH, 60, 0])

def test_stuck_note_prevention_on_chord_change(engine):
    # User holds D4 (key 62) on RH
    # Key index 2 -> degree 2 -> E4 (64)
    engine.process_message(([0x90, 62, 100], 0.0))

    # Engine tracks input note 62 mapping to output pitch 64
    assert 62 in engine._voices
    assert engine._voices[62].out_pitch == 64

    # User hits a new LH chord (changes root to G2)
    # LH_ZONE is 21-60. Note 43 is G2.
    engine.process_message(([0x90, 43, 100], 0.1))

    # Expectation: the engine MUST have sent a note-off for the currently sounding E4 (64)
    # BEFORE applying the new scale, to prevent a stuck note.
    engine.midi_out.send_message.assert_any_call([0x80 | config.RH_CH, 64, 0])

    # Voice should be cleared
    assert 62 not in engine._voices

    # Ensure new root is set
    assert engine.scale.root_midi == 43

    # User releases D4 (key 62) - it should silently do nothing or at least not crash
    engine.process_message(([0x80, 62, 0], 0.2))
    # Should not send note-off again since it was already released during scale change

def test_mpe_voice_allocation(mpe_engine):
    # Play 3 notes simultaneously
    mpe_engine.process_message(([0x90, 60, 100], 0.0))
    mpe_engine.process_message(([0x90, 61, 100], 0.0))
    mpe_engine.process_message(([0x90, 62, 100], 0.0))

    voices = mpe_engine._voices
    assert len(voices) == 3

    # They should have different channels
    channels = {v.out_channel for v in voices.values()}
    assert len(channels) == 3

    # All channels should be from the pool
    assert all(ch in config.MPE_CHANNEL_POOL for ch in channels)

def test_mpe_voice_stealing(mpe_engine):
    # Reduce pool to 2 channels to force stealing
    mpe_engine._mpe_pool = [1, 2]
    mpe_engine._mpe_alloc = {}

    # Play note 1
    mpe_engine.process_message(([0x90, 60, 100], 0.0))
    assert len(mpe_engine._voices) == 1
    ch1 = mpe_engine._voices[60].out_channel

    # Play note 2
    mpe_engine.process_message(([0x90, 61, 100], 0.1))
    assert len(mpe_engine._voices) == 2

    # Play note 3 (should steal oldest, which is note 60)
    mpe_engine.midi_out.reset_mock()
    mpe_engine.process_message(([0x90, 62, 100], 0.2))

    # Verify note 60 was turned off
    # Note 60 -> out_pitch 60
    mpe_engine.midi_out.send_message.assert_any_call([0x80 | ch1, 60, 0])

    # Note 60 should no longer be tracked
    assert 60 not in mpe_engine._voices
    assert len(mpe_engine._voices) == 2
    assert 62 in mpe_engine._voices

def test_cc_passthru(engine):
    # Send CC 1 (Mod Wheel) value 64
    engine.process_message(([0xB0, 1, 64], 0.0))

    # Should be passed to LH channel
    engine.midi_out.send_message.assert_called_with([0xB0 | config.LH_CH, 1, 64])
