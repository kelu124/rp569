"""Shared machinery for the firmware command lines: prompt-terminated request/response over a serial port."""
from dataclasses import dataclass
import re
import time

import serial

from . import ports

ANSI = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')
VERSION_RE = re.compile(r'rp569 (\S+) firmware (\d+)\.(\d+)\.(\d+) git (\S+) built (.+?) uid ([0-9A-Fa-f]+)')


def command(name: str):
    """Mark a method as the wrapper of firmware command `name` (checked by tools/check_commands.py)."""
    def mark(fn):
        fn.firmware_command = name
        return fn
    return mark


class CommandError(RuntimeError):
    pass


@dataclass
class FirmwareVersion:
    chip: str           # RP2350B / CH569W
    major: int
    minor: int
    patch: int
    git: str
    built: str
    uid: str            # chip unique ID, hex

    @property
    def version(self) -> str:
        return f'{self.major}.{self.minor}.{self.patch}'

    @classmethod
    def parse(cls, text: str) -> 'FirmwareVersion':
        m = VERSION_RE.search(text)
        if not m:
            raise CommandError(f'unexpected version reply: {text!r}')
        chip, ma, mi, pa, git, built, uid = m.groups()
        return cls(chip, int(ma), int(mi), int(pa), git, built.strip(), uid.upper())


class PromptCLI:
    """A firmware command line: send a line, collect output until the prompt comes back."""
    PROMPT = '> '
    ROLE = ''
    CHIP = ''                       # expected chip name in the version reply (identity check)

    def __init__(self, port: str | None = None, serial_number: str | None = None, timeout: float = 2.0,
                 strict: bool = True, check_identity: bool = True, transport=None):
        """port: device path, or None to find it by VID/PID/product (and serial_number = chip UID).
        transport: an object with read/write/close (tests); bypasses discovery and opening."""
        self.timeout = timeout
        if transport is not None:
            self.port = port or '<transport>'
            self.ser = transport
        else:
            if port is None:
                port = ports.find(self.ROLE, serial_number, strict).device
            else:
                ports.check_port(port, self.ROLE, strict)
            self.port = port
            self.ser = serial.Serial(port, 115200, timeout=0.05)
        self.async_lines: list[str] = []    # unsolicited lines seen while waiting (log, CH> ...)
        self._sync()
        if check_identity and self.CHIP:
            v = self.version()
            if v.chip != self.CHIP:
                self.close()
                raise ports.WrongDevice(f'{self.port} runs {v.chip} firmware, expected {self.CHIP}')

    # -- plumbing
    def close(self):
        self.ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def _read_some(self) -> str:
        data = self.ser.read(4096)
        return data.decode(errors='replace') if data else ''

    def _sync(self):
        """Get a fresh prompt: send an empty line and drain whatever was pending (banner, boot log)."""
        self.ser.write(b'\r')
        buf, end = '', time.time() + self.timeout
        while time.time() < end:
            buf += self._read_some()
            if self._clean(buf).endswith(self.PROMPT):
                break
        time.sleep(0.05)
        self._read_some()

    @staticmethod
    def _clean(text: str) -> str:
        # '\r ESC[2K' = the firmware clearing the prompt line before an unsolicited line: a line break here
        return ANSI.sub('', text.replace('\r\x1b[2K', '\n')).replace('\r', '')

    def _filter(self, line: str) -> bool:
        """True if the line is unsolicited output to keep aside (see subclasses)."""
        return False

    def _parse(self, text: str, line: str):
        """(done, output lines, unsolicited lines) for the text received since `line` was sent.

        Done = the echo of `line` has been seen and the text ends with a bare prompt. Unsolicited lines (CH>,
        log) may come before the echo, and the firmware re-prints the prompt after them: neither ends the wait."""
        parts = text.split('\n')
        echo_seen, out, unsolicited = False, [], []
        for l in parts[:-1]:
            if self._filter(l):
                unsolicited.append(l)
            elif l.startswith(self.PROMPT) and (echo_seen or l[len(self.PROMPT):] != line):
                continue                                  # prompt re-printed after an unsolicited line
            elif not echo_seen and l.endswith(line):
                echo_seen = True                          # the command as echoed by the firmware
            elif echo_seen and l:
                out.append(l)
        return echo_seen and parts[-1] == self.PROMPT, out, unsolicited

    def cmd(self, line: str, timeout: float | None = None, expect_prompt: bool = True) -> list[str]:
        """Run one command line, return its output lines (without the echo, prompts and unsolicited lines)."""
        self.ser.write(line.encode() + b'\r')
        if not expect_prompt:
            time.sleep(0.2)
            return []
        buf, end = '', time.time() + (timeout or self.timeout)
        while time.time() < end:
            buf += self._read_some()
            done, out, unsolicited = self._parse(self._clean(buf), line)
            if done:
                break
        else:
            raise CommandError(f'{self.port}: no prompt after {line!r} (got {self._clean(buf)!r})')
        self.async_lines += unsolicited
        if out and out[0].startswith('unknown command'):
            raise CommandError(out[0])
        return out

    def cmd_text(self, line: str, **kw) -> str:
        return '\n'.join(self.cmd(line, **kw))

    def _match(self, regex: str, line: str, **kw) -> re.Match:
        text = self.cmd_text(line, **kw)
        m = re.search(regex, text)
        if not m:
            raise CommandError(f'unexpected reply to {line!r}: {text!r}')
        return m

    def version(self) -> FirmwareVersion:
        raise NotImplementedError
