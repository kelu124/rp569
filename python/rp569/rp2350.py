"""RP2350B command line (firmware/rp2350): one method per firmware command.

Every entry of the firmware command table (firmware/rp2350/src/main.c, COMMANDS-BEGIN/END) must have a method
marked @command(<name>) here: tools/check_commands.py (pre-commit hook) enforces it.
"""
import re
import time

from ._cli import CommandError, FirmwareVersion, PromptCLI, command


class RP2350(PromptCLI):
    """The RP2350B USB serial command line. Opens the 'rp2350' port only (VID/PID + product + version check)."""
    PROMPT = 'rp569> '
    ROLE = 'rp2350'
    CHIP = 'RP2350B'
    COLOURS = 'RGB'

    def _filter(self, line):
        return line.startswith('CH> ') or line.startswith('CH569 power switch fault')

    def ch_lines(self) -> list[str]:
        """Lines received from the CH569 so far (without the 'CH> ' prefix), then forget them."""
        self.cmd_text('')                      # collect what arrived since the last command
        lines = [l[4:] for l in self.async_lines if l.startswith('CH> ')]
        self.async_lines = [l for l in self.async_lines if not l.startswith('CH> ')]
        return lines

    # -- board
    @command('help')
    def help(self) -> str:
        return self.cmd_text('help')

    @command('version')
    def version(self) -> FirmwareVersion:
        return FirmwareVersion.parse(self.cmd_text('version'))

    @command('set')
    def set_led(self, colour: str, on: bool) -> None:
        """One LED colour on/off: set_led('R', True)."""
        colour = colour.upper()
        if colour not in self.COLOURS or len(colour) != 1:
            raise ValueError('colour must be R, G or B')
        self._match(rf'{colour} (on|off)', f'set {colour} {int(bool(on))}')

    @command('set')
    def set_rgb(self, r: bool, g: bool, b: bool) -> None:
        """All three colours at once: set_rgb(1, 0, 1)."""
        self._match(r'R \d G \d B \d', f'set rgb {int(bool(r))}{int(bool(g))}{int(bool(b))}')

    @command('led')
    def leds(self) -> dict:
        m = self._match(r'R (\d) G (\d) B (\d)', 'led')
        return {c: v == '1' for c, v in zip(self.COLOURS, m.groups())}

    @command('btn')
    def buttons(self) -> dict:
        """{'BTN1': pressed, 'BTN2': pressed}"""
        m = self._match(r'BTN1 (up|down), BTN2 (up|down)', 'btn')
        return {'BTN1': m.group(1) == 'down', 'BTN2': m.group(2) == 'down'}

    @command('adc')
    def adc(self) -> dict:
        """{'VBUS3': volts, '3V3_CH': volts}"""
        m = self._match(r'VBUS3 ([\d.]+) V, 3V3_CH ([\d.]+) V', 'adc')
        return {'VBUS3': float(m.group(1)), '3V3_CH': float(m.group(2))}

    @command('dfu')
    def dfu(self) -> None:
        """Reboot into the RP2350 USB bootloader. The serial port disappears: the object is closed."""
        self.cmd('dfu', expect_prompt=False)
        self.close()

    @command('reboot')
    def reboot(self) -> None:
        """Reboot the RP2350. The serial port goes away for a moment: the object is closed."""
        self.cmd('reboot', expect_prompt=False)
        self.close()

    # -- CH569 control
    @command('ch on')
    def ch_on(self) -> str:
        return self.cmd_text('ch on', timeout=3)

    @command('ch off')
    def ch_off(self) -> str:
        return self.cmd_text('ch off', timeout=3)

    @command('ch cycle')
    def ch_cycle(self) -> str:
        return self.cmd_text('ch cycle', timeout=4)

    @command('ch boot')
    def ch_boot(self) -> str:
        """Power-cycle the CH569 into its USB bootloader (4348:55e0) for ~10 s."""
        return self.cmd_text('ch boot', timeout=4)

    @command('ch reset')
    def ch_reset(self) -> str:
        return self.cmd_text('ch reset')

    @command('ch status')
    def ch_status(self) -> dict:
        m = self._match(r'CH569 power (on|off), FLG# (ok|FAULT), 3V3_CH ([\d.]+) V, VBUS3 ([\d.]+) V', 'ch status')
        return {'power': m.group(1) == 'on', 'fault': m.group(2) == 'FAULT',
                '3V3_CH': float(m.group(3)), 'VBUS3': float(m.group(4))}

    @command('ch send')
    def ch_send(self, text: str, wait_echo: bool = True, timeout: float = 1.0) -> str | None:
        """Send a line to the CH569. With wait_echo, return the line the CH569 echoed back (None if none)."""
        if '\n' in text or '\r' in text:
            raise ValueError('one line at a time')
        out = self.cmd_text(f'ch send {text}')
        if out:
            raise CommandError(out)                    # 'CH569 is off' / usage
        if not wait_echo:
            return None
        end = time.time() + timeout
        while time.time() < end:
            for i, l in enumerate(self.async_lines):
                if l == f'CH> {text}':
                    del self.async_lines[i]
                    return text
            time.sleep(0.05)
            self.cmd_text('')
        return None
