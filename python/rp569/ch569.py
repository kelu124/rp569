"""CH569W ports (firmware/ch569): the console (second CDC port) and the RP2350 link bridge (first CDC port).

Every entry of the console command table (firmware/ch569/src/console.c, COMMANDS-BEGIN/END) must have a method
marked @command(<name>) here: tools/check_commands.py (pre-commit hook) enforces it.
"""
import re
import time

import serial

from . import ports
from ._cli import CommandError, FirmwareVersion, PromptCLI, command


class CH569Console(PromptCLI):
    """CH569 debug port: debug log + console. Opens the 'ch569-console' port only."""
    PROMPT = 'ch569> '
    ROLE = 'ch569-console'
    CHIP = 'CH569W'

    def _filter(self, line):
        return False                # log lines interleave with replies; the methods match what they need

    @command('help')
    def help(self) -> str:
        return self.cmd_text('help')

    @command('version')
    def version(self) -> FirmwareVersion:
        return FirmwareVersion.parse(self.cmd_text('version'))

    @command('status')
    def status(self) -> dict:
        m = self._match(r'usb (\w+), sd (\w+) \((\d+) MB\), link UART1 (\d+) baud, lines from RP2350 (\d+), '
                        r'echo (on|off)(?:, sd clock (\d+) MHz)?(?:, mode (drive|serial))?', 'status')
        return {'usb': m.group(1), 'sd_ready': m.group(2) == 'ready', 'sd_mb': int(m.group(3)),
                'link_baud': int(m.group(4)), 'link_lines': int(m.group(5)), 'echo': m.group(6) == 'on',
                'sd_clock_mhz': int(m.group(7)) if m.group(7) else None, 'mode': m.group(8)}

    @command('sd')
    def sd_info(self) -> dict | None:
        """SD card identity and bus: the CID fields (SD Physical Layer spec 5.2), size, clock; None before any mount.
        In serial mode it describes the card last mounted (state 'released')."""
        text = self.cmd_text('sd')
        if 'sd none' in text:
            return None
        m = re.search(r'sd cid ([0-9a-fA-F]{32}) size (\d+) MB clock (\d+) MHz (\w+)', text)
        if not m:
            raise CommandError(f'unexpected reply to sd: {text!r}')
        info = decode_cid(m.group(1))
        info.update(size_mb=int(m.group(2)), clock_mhz=int(m.group(3)), state=m.group(4))
        return info

    @command('mount')
    def mount(self) -> None:
        """Switch to drive mode: the CH569 mounts the card and re-enumerates as the drive; this port disappears,
        so the reply is not waited for (close the port afterwards; RP2350.ch_mount() reports the result)."""
        self.cmd('mount', expect_prompt=False)


def decode_cid(hex32: str) -> dict:
    """SD CID register (128 bits, hex, bit 127 first) -> fields. SD Physical Layer spec 5.2:
    MID [127:120], OID [119:104], PNM [103:64], PRV [63:56], PSN [55:24], MDT [19:8] (year 2000 + [19:12], month [11:8])."""
    v = int(hex32, 16)
    def bits(hi, lo):
        return (v >> lo) & ((1 << (hi - lo + 1)) - 1)
    def text(n, nbytes):
        return bytes((n >> (8 * (nbytes - 1 - i))) & 0xFF for i in range(nbytes)).decode('ascii', 'replace').strip()
    prv = bits(63, 56)
    mdt = bits(19, 8)
    return {'cid': hex32.lower(), 'mid': bits(127, 120), 'oid': text(bits(119, 104), 2), 'name': text(bits(103, 64), 5),
            'rev': f'{prv >> 4}.{prv & 0xF}', 'serial': f'{bits(55, 24):08x}',
            'date': f'{2000 + (mdt >> 4)}-{mdt & 0xF:02d}'}


class CH569Link:
    """CH569 first CDC port: raw bridge to the RP2350 link UART (no command line).

    What the RP2350 sends ('ch send ...') arrives here; what is written here goes to the RP2350, which prints it
    as 'CH> ...'. Opens the 'ch569-link' port only."""
    ROLE = 'ch569-link'

    def __init__(self, port: str | None = None, serial_number: str | None = None, strict: bool = True,
                 timeout: float = 1.0, transport=None):
        if transport is not None:
            self.port, self.ser = port or '<transport>', transport
        else:
            if port is None:
                port = ports.find(self.ROLE, serial_number, strict).device
            else:
                ports.check_port(port, self.ROLE, strict)
            self.port = port
            self.ser = serial.Serial(port, 115200, timeout=0.05)
        self.timeout = timeout
        self._buf = ''

    def close(self):
        self.ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def write_line(self, text: str) -> None:
        self.ser.write(text.encode() + b'\r\n')

    def read_line(self, timeout: float | None = None) -> str | None:
        """Next line from the RP2350 (without CR/LF), or None after the timeout."""
        end = time.time() + (timeout or self.timeout)
        while '\n' not in self._buf and time.time() < end:
            data = self.ser.read(1024)
            self._buf += data.decode(errors='replace') if data else ''
        if '\n' not in self._buf:
            return None
        line, self._buf = self._buf.split('\n', 1)
        return re.sub(r'\r', '', line)
