import time
import rtmidi
from neoharp.engine import NeoHarpEngine
from neoharp import config

def run_neoharp(input_port: int = 0, output_port: int = 0,
                scale: str = "maqam_rast", root: int = 60, mpe: bool = False) -> None:
    midi_in  = rtmidi.MidiIn()
    midi_out = rtmidi.MidiOut()

    # Get available ports
    in_ports = midi_in.get_ports()
    out_ports = midi_out.get_ports()

    if not in_ports or not out_ports:
        print("No MIDI ports available. Please ensure virtual or hardware MIDI ports are configured.")
        return

    midi_in.open_port(input_port)
    midi_out.open_port(output_port)

    engine = NeoHarpEngine(scale_name=scale, root=root, mpe=mpe)
    engine.midi_out = midi_out

    # Set pitch bend range on RH channel(s) to +/- 200 cents (2 semitones)
    def set_pb_range(ch):
        midi_out.send_message([0xB0 | ch, 101, 0])  # RPN MSB
        midi_out.send_message([0xB0 | ch, 100, 0])  # RPN LSB
        midi_out.send_message([0xB0 | ch,   6, 2])  # Data Entry: 2 semitones

    if mpe:
        for ch in config.MPE_CHANNEL_POOL:
            set_pb_range(ch)
    else:
        set_pb_range(config.RH_CH)

    midi_in.set_callback(engine.process_message)
    midi_in.ignore_types(sysex=False, timing=True, active_sense=True)

    print(f"NeoHarp running | Scale: {scale} | Root: {root} | MPE: {mpe} | Ctrl+C to stop")
    try:
        while True:
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nStopping NeoHarp...")
    finally:
        midi_in.close_port()
        midi_out.close_port()
        del midi_in
        del midi_out

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="NeoHarp PoC")
    parser.add_argument("--scale", type=str, default="maqam_rast", help="Starting scale name")
    parser.add_argument("--root", type=int, default=60, help="Starting root MIDI note")
    parser.add_argument("--mpe", action="store_true", help="Enable MPE mode")
    parser.add_argument("--in-port", type=int, default=0, help="MIDI input port index")
    parser.add_argument("--out-port", type=int, default=0, help="MIDI output port index")

    args = parser.parse_args()
    run_neoharp(input_port=args.in_port, output_port=args.out_port,
                scale=args.scale, root=args.root, mpe=args.mpe)
