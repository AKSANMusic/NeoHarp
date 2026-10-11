# neoharp/strum.py

import threading
import time
from typing import List

from neoharp.engine import NeoHarpEngine

def strum(engine: NeoHarpEngine, input_notes: List[int],
          direction: str = "down", interval_ms: float = 10.0,
          velocity_start: int = 100, velocity_decay: int = 4) -> None:
    """
    Triggers a list of RH input notes with timed offsets to simulate strumming.
    direction: 'down' (high->low) or 'up' (low->high)
    """
    ordered = sorted(input_notes, reverse=(direction == "down"))

    def _run():
        for i, note in enumerate(ordered):
            vel = max(1, velocity_start - i * velocity_decay)
            # Directly call internal note_on mapping for simulated strumming
            engine._note_on(note, vel)
            time.sleep(interval_ms / 1000.0)

    # Starts in a background thread to allow asynchronous scheduling
    threading.Thread(target=_run, daemon=True).start()
