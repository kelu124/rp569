"""python -m rp569 [ports|versions|eject|mount]: list the rp569 serial ports / print every chip's firmware version /
eject: unmount the CH569 SD drive on this host, then the CH569 releases the card and switches to serial mode /
mount: the CH569 mounts the card and switches back to drive mode (its serial ports go away)."""
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
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
