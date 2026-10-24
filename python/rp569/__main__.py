"""python -m rp569 [ports|versions]: list the rp569 serial ports / print every chip's firmware version."""
import sys

from . import __version__, ports
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
    elif args[0] == 'versions':
        for p in found:
            cls = {'rp2350': RP2350, 'ch569-console': CH569Console}.get(p.role)
            if cls:
                with cls(p.device) as dev:
                    v = dev.version()
                    print(f'{p.role:14s} {p.device:16s} {v.chip} {v.version} git {v.git} uid {v.uid}')
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
