"""The CH569's SD-card drive on the host (Linux): find its partitions, unmount them cleanly.

Releasing the card from the board side (RP2350 `ch eject`, done before every CH569 power-off) makes the card itself
safe, but the host only marks a FAT volume clean when it unmounts it. `eject()` does both, in that order:
sync + `udisksctl unmount` of every mounted partition of the CH569 drive, then `ch eject` on the RP2350.
"""
import os
import subprocess

CH569_VID, CH569_PID = '1a86', 'fe2c'


def _udev_props(dev: str) -> dict:
    out = subprocess.run(['udevadm', 'info', '-q', 'property', '-n', dev],
                         capture_output=True, text=True, check=False).stdout
    return dict(line.split('=', 1) for line in out.splitlines() if '=' in line)


def _parse_lsblk(text: str) -> list[tuple[str, str]]:
    """(device, mountpoint) for each `lsblk -nrpo NAME,MOUNTPOINT` line (mountpoint '' when not mounted)."""
    rows = []
    for line in text.splitlines():
        name, _, mnt = line.partition(' ')
        if name:
            rows.append((name, mnt.replace('\\x20', ' ')))
    return rows


def partitions(serial: str | None = None, props=_udev_props) -> list[tuple[str, str]]:
    """(partition, mountpoint) of the CH569 drive(s), optionally only the board whose USB serial is `serial`."""
    text = subprocess.run(['lsblk', '-nrpo', 'NAME,MOUNTPOINT'], capture_output=True, text=True,
                          check=False).stdout
    out = []
    for dev, mnt in _parse_lsblk(text):
        p = props(dev)
        if p.get('ID_VENDOR_ID') != CH569_VID or p.get('ID_MODEL_ID') != CH569_PID or p.get('DEVTYPE') != 'partition':
            continue
        if serial and (p.get('ID_SERIAL_SHORT') or '').upper() != serial.upper():
            continue
        out.append((dev, mnt))
    return out


def unmount(serial: str | None = None) -> list[str]:
    """Sync, then unmount every mounted partition of the CH569 drive. Returns the partitions unmounted."""
    os.sync()
    done = []
    for dev, mnt in partitions(serial):
        if mnt:
            subprocess.run(['udisksctl', 'unmount', '-b', dev], check=True, capture_output=True)
            done.append(dev)
    return done


def eject(rp, serial: str | None = None) -> tuple[list[str], bool]:
    """Clean eject: host unmount (FAT marked clean), then the RP2350 asks the CH569 to release the card.
    `rp` is an open rp569.RP2350. Returns (partitions unmounted, CH569 acknowledged)."""
    done = unmount(serial)
    return done, rp.ch_eject()
