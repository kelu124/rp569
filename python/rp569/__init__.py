"""Host library for the rp569 board: RP2350B command line, CH569W console and RP2350-link bridge.

    import rp569
    with rp569.RP2350() as rp:           # finds the port by VID/PID 2e8a:0009 + product "rp569 RP2350B"
        print(rp.version())               # FirmwareVersion(chip='RP2350B', major=0, ..., uid='E661...')
        rp.set_led('R', True)
        print(rp.ch_send('hello'))         # 'hello', echoed by the CH569
"""
from ._version import __version__
from ._cli import CommandError, FirmwareVersion, command
from .ch569 import CH569Console, CH569Link
from .ports import PortNotFound, WrongDevice, find, list_ports_rp569
from .rp2350 import RP2350

__all__ = ['__version__', 'RP2350', 'CH569Console', 'CH569Link', 'FirmwareVersion', 'CommandError',
           'PortNotFound', 'WrongDevice', 'find', 'list_ports_rp569', 'command']
