"""CH569W ports (firmware/ch569): the console (second CDC port) and the RP2350 link bridge (first CDC port).

Every entry of the console command table (firmware/ch569/src/console.c, COMMANDS-BEGIN/END) must have a method
marked @command(<name>) here: tools/check_commands.py (pre-commit hook) enforces it.
"""
import re
import time

import serial

from . import ports
from ._cli import FirmwareVersion, PromptCLI, command


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
                        r'echo (on|off)', 'status')
        return {'usb': m.group(1), 'sd_ready': m.group(2) == 'ready', 'sd_mb': int(m.group(3)),
                'link_baud': int(m.group(4)), 'link_lines': int(m.group(5)), 'echo': m.group(6) == 'on'}


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
