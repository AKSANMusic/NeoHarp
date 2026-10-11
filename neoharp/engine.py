# neoharp/engine.py

import threading
from dataclasses import dataclass
from typing import Optional
import rtmidi

from neoharp import config
from neoharp.scales import Scale, SCALES


@dataclass
class ActiveVoice:
    input_note: int
    out_channel: int   # 0-indexed
    out_pitch: int
    out_pb: int
    velocity: int


class NeoHarpEngine:
    def __init__(self, scale_name: str = "major", root: int = 60, mpe: bool = False):
        self.scale = Scale(root, SCALES[scale_name], pb_range_cents=config.PB_RANGE_CENTS)
        self.mpe_mode = mpe

        self._voices: dict[int, ActiveVoice] = {}   # input_note -> ActiveVoice
        self._lock = threading.Lock()

        # MPE channel pool
        self._mpe_pool: list[int] = config.MPE_CHANNEL_POOL.copy()
        self._mpe_alloc: dict[int, int] = {}        # input_note -> channel

        self.midi_out: Optional[rtmidi.MidiOut] = None

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def set_scale(self, root: int, scale_name: str) -> None:
        """
        Change harmonic context. Releases all active RH voices on their
        current output pitches before applying the new scale.
        """
        with self._lock:
            self._release_all_rh_voices()
            self.scale = Scale(root, SCALES[scale_name], pb_range_cents=config.PB_RANGE_CENTS)

    def set_root(self, root: int) -> None:
        with self._lock:
            self._release_all_rh_voices()
            self.scale.root_midi = root

    def process_message(self, event, data=None) -> None:
        """rtmidi callback — runs on C++ thread, keep lean."""
        if not event:
            return

        # python-rtmidi passes a tuple: (message_list, delta_time)
        if isinstance(event, tuple):
            msg, _timestamp = event
        else:
            msg = event

        if not msg:
            return

        status = msg[0] & 0xF0
        note   = msg[1] if len(msg) > 1 else 0
        vel    = msg[2] if len(msg) > 2 else 0

        if status == 0x90 and vel > 0:
            self._note_on(note, vel)
        elif status == 0x80 or (status == 0x90 and vel == 0):
            self._note_off(note)
        elif status == 0xB0:
            self._cc_passthru(msg)

    # ------------------------------------------------------------------
    # Internal handlers
    # ------------------------------------------------------------------

    def _note_on(self, note: int, velocity: int) -> None:
        if note in config.LH_ZONE:
            # Left hand: update harmonic root and send bass note
            with self._lock:
                self._release_all_rh_voices()
                self.scale.root_midi = note
            self._send(0x90 | config.LH_CH, note, velocity)

        elif note in config.RH_ZONE:
            with self._lock:
                idx     = note - config.RH_ZONE.start
                degree  = config.RH_KEY_TO_DEGREE[idx % len(config.RH_KEY_TO_DEGREE)]
                octave  = idx // len(config.RH_KEY_TO_DEGREE)

                out_pitch, pb = self.scale.degree_to_pitch_event(degree, octave)
                out_ch        = self._alloc_channel(note)

                voice = ActiveVoice(note, out_ch, out_pitch, pb, velocity)
                self._voices[note] = voice

                # Set pitch bend before note-on to avoid audible glitch
                if pb != 0:
                    self._pitch_bend(out_ch, pb)
                self._send(0x90 | out_ch, out_pitch, velocity)

    def _note_off(self, note: int) -> None:
        if note in config.LH_ZONE:
            self._send(0x80 | config.LH_CH, note, 0)

        elif note in config.RH_ZONE:
            with self._lock:
                voice = self._voices.pop(note, None)
                if voice:
                    self._free_channel(note)
            if voice:
                self._send(0x80 | voice.out_channel, voice.out_pitch, 0)

    def _cc_passthru(self, msg: list[int]) -> None:
        # Route CC to LH channel by default; extend for RH CCs if needed
        self._send(0xB0 | config.LH_CH, msg[1], msg[2])

    # ------------------------------------------------------------------
    # Voice / channel allocation
    # ------------------------------------------------------------------

    def _alloc_channel(self, input_note: int) -> int:
        if not self.mpe_mode:
            return config.RH_CH
        if self._mpe_pool:
            ch = self._mpe_pool.pop(0)
        else:
            # Voice steal: take channel from oldest active voice
            oldest_note = next(iter(self._voices))
            stolen = self._voices.pop(oldest_note)
            self._send(0x80 | stolen.out_channel, stolen.out_pitch, 0)
            ch = stolen.out_channel
            self._mpe_alloc.pop(oldest_note, None)
        self._mpe_alloc[input_note] = ch
        return ch

    def _free_channel(self, input_note: int) -> None:
        if self.mpe_mode:
            ch = self._mpe_alloc.pop(input_note, None)
            if ch is not None:
                self._mpe_pool.append(ch)

    def _release_all_rh_voices(self) -> None:
        """Send Note-Off for every active RH voice. Call inside lock."""
        for voice in self._voices.values():
            self._send(0x80 | voice.out_channel, voice.out_pitch, 0)
            if self.mpe_mode:
                self._mpe_alloc.pop(voice.input_note, None)
                self._mpe_pool.append(voice.out_channel)
        self._voices.clear()

    # ------------------------------------------------------------------
    # MIDI output helpers
    # ------------------------------------------------------------------

    def _send(self, status: int, d1: int, d2: int) -> None:
        if self.midi_out:
            self.midi_out.send_message([status, d1 & 0x7F, d2 & 0x7F])

    def _pitch_bend(self, channel: int, pb_14bit: int) -> None:
        """pb_14bit: signed -8192..+8191"""
        unsigned = pb_14bit + 8192       # → 0..16383
        self._send(0xE0 | channel, unsigned & 0x7F, (unsigned >> 7) & 0x7F)
