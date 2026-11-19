"""python -m rp569 [ports|versions|eject|mount|stream [seconds] [MB/s]]: list the rp569 serial ports / print every chip's firmware version /
eject: unmount the CH569 SD drive on this host, then the CH569 releases the card and switches to serial mode /
mount: the CH569 mounts the card and switches back to drive mode (its serial ports go away) /
stream: firmware variant 1: RP2350 counter stream over the 8-bit bus, read and checked from the CH569 bulk endpoint."""
import sys

from . import __version__, drive, ports
from .ch569 import CH569Console
from .rp2350 import RP2350


def main(argv=None):
    args = (argv if argv is not None else sys.argv[1:]) or ['ports']
    print(f'rp569 python library {__version__}')
    found = ports.list_ports_rp569()
    if args[0] == 'ports':
        for p in found:
            print(f'{p.role:14s} {p.device:16s} serial {p.serial_number}  product {p.product!r}  if {p.interface}')
        if not found:
            print('no rp569 port found')
        print(f'ch569 USB mode: {ports.ch569_usb_mode() or "not on USB"}')
    elif args[0] == 'versions':
        for p in found:
            cls = {'rp2350': RP2350, 'ch569-console': CH569Console}.get(p.role)
            if cls:
                with cls(p.device) as dev:
                    v = dev.version()
                    print(f'{p.role:14s} {p.device:16s} {v.chip} {v.version} git {v.git} uid {v.uid}')
    elif args[0] == 'eject':
        rp = next((p for p in found if p.role == 'rp2350'), None)
        if rp is None:
            sys.exit('no RP2350 port found')
        ch = next((p for p in found if p.role.startswith('ch569')), None)
        with RP2350(rp.device) as dev:
            done, ack = drive.eject(dev, ch.serial_number if ch else None)
        print(f"unmounted: {', '.join(done) or 'nothing mounted'}; CH569 {'released the card' if ack else 'did not answer'}")
    elif args[0] == 'mount':
        rp = next((p for p in found if p.role == 'rp2350'), None)
        if rp is None:
            sys.exit('no RP2350 port found')
        with RP2350(rp.device) as dev:
            ok = dev.ch_mount()
        print(f"CH569 {'mounted the card (drive mode)' if ok else 'did not mount the card'}")
    elif args[0] == 'stream':
        from .stream import StreamReader
        seconds = float(args[1]) if len(args) > 1 else 5.0
        mbs = float(args[2]) if len(args) > 2 else 16.0
        rp = next((p for p in found if p.role == 'rp2350'), None)
        if rp is None:
            sys.exit('no RP2350 port found')
        with StreamReader() as s, RP2350(rp.device) as dev:
            dev.stream_stop()
            s.drain()
            s.reset()
            started = {}
            res = s.measure(seconds, start=lambda: started.update(dev.stream_start(mbs)))
            start = started
            stop = dev.stream_stop()
            ch = s.stats()
        print(f"RP2350 sent   {stop['packets']} packets, {stop['bytes']} B in {stop['seconds']:.2f} s = "
              f"{stop['mbs']:.2f} MB/s (target {start['target_mbs']:g}, bus {start['bus_mhz']:.1f} MHz)")
        print(f"CH569 got     {ch['packets']} packets, {ch['bytes_in']} B; sent {ch['bytes_out']} B; "
              f"crc_err {ch['crc_errors']} seq_err {ch['seq_errors']} overflow {ch['overflows']} fifo_ov {ch['fifo_ov']}")
        print(f"host received {res['bytes']} B in {res['seconds']:.2f} s = {res['mbs']:.2f} MB/s; "
              f"gaps {res['gaps']}, words lost {res['words_lost']}, first word {res['first_word']}")
        if res['gap_at']:
            print('gaps at (word, words lost, s since start):', res['gap_at'])
        sys.exit(0 if res['bytes'] and not res['gaps'] else 1)
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
