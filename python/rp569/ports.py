"""Find the rp569 serial ports by USB VID/PID, product string, serial number and interface number.

Each port gets a role, so a class never opens the other chip's port:

| Role | USB device | Interface | What it is |
|---|---|---|---|
| ``rp2350`` | 2e8a:0009, product "rp569 RP2350B" | CDC | RP2350B command line |
| ``ch569-link`` | 1a86:fe2c, product "rp569 CH569" | 0 (first CDC) | RP2350 link UART bridge |
| ``ch569-console`` | 1a86:fe2c, product "rp569 CH569" | 2 (second CDC) | CH569 debug log + console |

The CH569 shows these two ports only in serial mode; in drive mode it is the SD card drive alone (no serial port).
Firmware before the two modes (0.1.4 and older) put them on interfaces 1 and 3, after the drive: still accepted.

The USB serial number of both chips is their unique ID (same value as the ``uid`` of the ``version`` command),
so ``find(..., serial=uid)`` picks one board when several are plugged in.
"""
from dataclasses import dataclass
import re

from serial.tools import list_ports

RP2350_VID, RP2350_PID = 0x2E8A, 0x0009      # Raspberry Pi, Pico SDK stdio_usb CDC
RP2350_PRODUCT = 'rp569 RP2350B'
CH569_VID, CH569_PID = 0x1A86, 0xFE2C         # WCH, rp569 / CH569_Composite PID
CH569_PRODUCT = 'rp569 CH569'
CH569_LINK_IF, CH569_CONSOLE_IF = 0, 2       # CDC communication interfaces in serial mode
CH569_LINK_IFS, CH569_CONSOLE_IFS = (0, 1), (2, 3)   # + 1 / 3 of the older composite firmware

# Bootloaders (no serial port): shown by list_devices() for information
RP2350_BOOT = (0x2E8A, 0x000F)               # RP2350 BOOTSEL (after 'dfu')
CH569_BOOT = (0x4348, 0x55E0)                # WCH ISP bootloader (after 'ch boot')

ROLES = ('rp2350', 'ch569-link', 'ch569-console')


class PortNotFound(RuntimeError):
    pass


class WrongDevice(RuntimeError):
    """The port belongs to another device or role than the one asked for."""


@dataclass
class Port:
    role: str
    device: str                  # /dev/ttyACM0, COM5, ...
    serial_number: str | None    # chip unique ID
    product: str | None
    interface: int | None
    location: str | None


def interface_number(info) -> int | None:
    """USB interface number from pyserial's location ('1-2.3:1.1' on Linux, '...:x.1' on Windows)."""
    m = re.search(r':(?:\d+|x)\.(\d+)$', info.location or '')
    return int(m.group(1)) if m else None


def role_of(info, strict: bool = True) -> str | None:
    """Role of one pyserial ListPortInfo, or None if it is not an rp569 port.

    strict=False also accepts a stock Pico SDK CDC firmware / the original CH569_Composite product strings."""
    product = info.product or ''
    if (info.vid, info.pid) == (RP2350_VID, RP2350_PID):
        if not strict or RP2350_PRODUCT in product:
            return 'rp2350'
    if (info.vid, info.pid) == (CH569_VID, CH569_PID):
        if strict and CH569_PRODUCT not in product:
            return None
        n = interface_number(info)
        if n in CH569_LINK_IFS:
            return 'ch569-link'
        if n in CH569_CONSOLE_IFS:
            return 'ch569-console'
        return 'ch569-?'        # interface number unknown on this OS: see ports_for()
    return None


def list_ports_rp569(strict: bool = True, infos=None) -> list[Port]:
    """Every rp569 serial port currently plugged in."""
    infos = list(list_ports.comports()) if infos is None else list(infos)
    out = []
    for info in infos:
        role = role_of(info, strict)
        if role:
            out.append(Port(role, info.device, info.serial_number, info.product, interface_number(info),
                            info.location))
    # OS without interface numbers in the location: order the two CH569 ports of one chip by device name
    unknown = sorted((p for p in out if p.role == 'ch569-?'), key=lambda p: (p.serial_number or '', p.device))
    for i, p in enumerate(unknown):
        p.role = 'ch569-link' if i % 2 == 0 else 'ch569-console'
    return out


def ch569_usb_mode(sysfs: str = '/sys/bus/usb/devices') -> str | None:
    """Mode of the CH569 read from its USB enumeration (Linux sysfs): 'drive' / 'serial' from the product string
    ("rp569 CH569 drive|serial"), 'bootloader' for the WCH ISP device, None if no CH569 is on USB. Works in drive
    mode too, where the CH569 has no serial port."""
    import glob
    import os

    def read(d, name):
        try:
            with open(os.path.join(d, name)) as f:
                return f.read().strip()
        except OSError:
            return ''
    for d in glob.glob(os.path.join(sysfs, '*')):
        ids = (read(d, 'idVendor').lower(), read(d, 'idProduct').lower())
        if ids == ('1a86', 'fe2c'):
            product = read(d, 'product')
            if product.startswith(CH569_PRODUCT):
                rest = product[len(CH569_PRODUCT):].strip()
                return rest or 'composite'        # firmware before the two modes: drive + serial at once
        if ids == ('4348', '55e0'):
            return 'bootloader'
    return None


def find(role: str, serial: str | None = None, strict: bool = True, infos=None) -> Port:
    """The one port with this role (and chip UID if given). Raises PortNotFound if none or ambiguous."""
    if role not in ROLES:
        raise ValueError(f'role must be one of {ROLES}')
    ports = [p for p in list_ports_rp569(strict, infos) if p.role == role]
    if serial:
        ports = [p for p in ports if (p.serial_number or '').upper() == serial.upper()]
    if not ports:
        raise PortNotFound(f'no {role} port found' + (f' with serial {serial}' if serial else '') +
                           ' (plugged in? the CH569 shows its serial ports only in serial mode: eject the drive or run ch eject)')
    if len(ports) > 1:
        raise PortNotFound(f'{len(ports)} {role} ports found, pass serial=<uid>: ' +
                           ', '.join(f'{p.device} ({p.serial_number})' for p in ports))
    return ports[0]


def check_port(device: str, role: str, strict: bool = True, infos=None) -> None:
    """Raise WrongDevice if an explicitly given port is a known rp569 port with another role."""
    for p in list_ports_rp569(strict, infos):
        if p.device == device and p.role != role:
            raise WrongDevice(f'{device} is the {p.role} port, not {role}')
