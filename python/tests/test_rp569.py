"""Tests without hardware: port discovery on fake port lists, command parsing on a fake firmware.

    cd python && python3 -m unittest -v
"""
from types import SimpleNamespace
import unittest

import rp569
from rp569 import ports
from rp569._cli import FirmwareVersion

UID_RP = 'E6614103E7A1B22C'
UID_CH = '0123456789ABCDEF'


def info(device, vid, pid, product, serial, location):
    return SimpleNamespace(device=device, vid=vid, pid=pid, product=product, serial_number=serial,
                           location=location, interface=None)


BOARD = [
    info('/dev/ttyACM0', 0x2E8A, 0x0009, 'rp569 RP2350B', UID_RP, '1-2:1.0'),
    info('/dev/ttyACM1', 0x1A86, 0xFE2C, 'rp569 CH569', UID_CH, '2-1:1.1'),
    info('/dev/ttyACM2', 0x1A86, 0xFE2C, 'rp569 CH569', UID_CH, '2-1:1.3'),
    info('/dev/ttyACM3', 0x2E8A, 0x0009, 'Pico', 'AAAA', '1-3:1.0'),         # some other Pico: not ours
    info('/dev/ttyUSB0', 0x0403, 0x6001, 'FT232R', 'X', '1-4:1.0'),
]


class FakeFirmware:
    """Echoes typed characters, answers commands like the firmware, then prints the prompt."""

    def __init__(self, prompt, replies, async_after=None):
        self.prompt, self.replies, self.async_after = prompt, replies, async_after or {}
        self.out, self.line, self.closed = '', '', False

    def write(self, data):
        for ch in data.decode():
            if ch == '\r':
                cmd, self.line = self.line, ''
                reply = self.replies.get(cmd, '' if cmd == '' else f"unknown command '{cmd}' (try 'help')")
                self.out += '\n' + (reply + '\n' if reply else '') + self.prompt
                for text in self.async_after.get(cmd, []):
                    self.out += f'\r\x1b[2K{text}\n' + self.prompt
            else:
                self.line += ch
                self.out += ch

    def read(self, n):
        data, self.out = self.out[:n], self.out[n:]
        return data.encode()

    def close(self):
        self.closed = True


RP_REPLIES = {
    'version': f'rp569 RP2350B firmware 0.1.3 git v1.0-9-g0727cb0 built Oct 24 2026 uid {UID_RP}',
    'set R 1': 'R on', 'set rgb 101': 'R 1 G 0 B 1', 'led': 'R 1 G 0 B 1',
    'btn': 'BTN1 up, BTN2 down', 'adc': 'VBUS3 5.02 V, 3V3_CH 3.31 V',
    'ch status': 'CH569 power on, FLG# ok, 3V3_CH 3.30 V, VBUS3 5.01 V, link UART1 115200 baud',
    'ch send hello': '',
}
CH_REPLIES = {
    'version': f'rp569 CH569W firmware 0.1.2 git 0727cb0 built Oct 24 2026 uid {UID_CH}',
    'status': 'usb usb3, sd ready (30436 MB), link UART1 115200 baud, lines from RP2350 4, echo on',
}


class TestPorts(unittest.TestCase):
    def test_roles(self):
        found = {p.device: p.role for p in ports.list_ports_rp569(infos=BOARD)}
        self.assertEqual(found, {'/dev/ttyACM0': 'rp2350', '/dev/ttyACM1': 'ch569-link',
                                 '/dev/ttyACM2': 'ch569-console'})

    def test_not_strict_accepts_other_pico(self):
        self.assertEqual(len([p for p in ports.list_ports_rp569(strict=False, infos=BOARD) if p.role == 'rp2350']), 2)

    def test_find_by_serial(self):
        self.assertEqual(ports.find('ch569-console', serial=UID_CH.lower(), infos=BOARD).device, '/dev/ttyACM2')
        with self.assertRaises(ports.PortNotFound):
            ports.find('rp2350', serial='nope', infos=BOARD)

    def test_wrong_port_refused(self):
        with self.assertRaises(ports.WrongDevice):
            ports.check_port('/dev/ttyACM1', 'rp2350', infos=BOARD)
        ports.check_port('/dev/ttyACM0', 'rp2350', infos=BOARD)

    def test_no_interface_numbers(self):
        mac = [info('/dev/cu.usbmodem3', 0x1A86, 0xFE2C, 'rp569 CH569', UID_CH, '20-2'),
               info('/dev/cu.usbmodem1', 0x1A86, 0xFE2C, 'rp569 CH569', UID_CH, '20-2')]
        roles = {p.device: p.role for p in ports.list_ports_rp569(infos=mac)}
        self.assertEqual(roles, {'/dev/cu.usbmodem1': 'ch569-link', '/dev/cu.usbmodem3': 'ch569-console'})


class TestRP2350(unittest.TestCase):
    def rp(self, **kw):
        return rp569.RP2350(transport=FakeFirmware('rp569> ', RP_REPLIES, **kw))

    def test_version(self):
        v = self.rp().version()
        self.assertEqual((v.chip, v.version, v.uid), ('RP2350B', '0.1.3', UID_RP))

    def test_led_btn_adc(self):
        rp = self.rp()
        rp.set_led('r', True)
        rp.set_rgb(1, 0, 1)
        self.assertEqual(rp.leds(), {'R': True, 'G': False, 'B': True})
        self.assertEqual(rp.buttons(), {'BTN1': False, 'BTN2': True})
        self.assertEqual(rp.adc(), {'VBUS3': 5.02, '3V3_CH': 3.31})
        self.assertTrue(rp.ch_status()['power'])

    def test_ch_send_echo(self):
        rp = self.rp(async_after={'ch send hello': ['CH> hello']})
        self.assertEqual(rp.ch_send('hello'), 'hello')

    def test_async_line_before_echo(self):
        fw = FakeFirmware('rp569> ', RP_REPLIES)
        rp = rp569.RP2350(transport=fw)
        fw.out += '\r\x1b[2KCH> early\n' + fw.prompt        # arrives before the next command's echo
        self.assertEqual(rp.leds()['R'], True)
        self.assertEqual(rp.ch_lines(), ['early'])

    def test_unknown_command(self):
        with self.assertRaises(rp569.CommandError):
            self.rp().cmd('bogus')

    def test_identity_check(self):
        with self.assertRaises(rp569.WrongDevice):
            rp569.RP2350(transport=FakeFirmware('rp569> ', {'version': CH_REPLIES['version']}))


class TestCH569(unittest.TestCase):
    def test_console(self):
        con = rp569.CH569Console(transport=FakeFirmware('ch569> ', CH_REPLIES))
        self.assertEqual(con.version().uid, UID_CH)
        st = con.status()
        self.assertEqual((st['usb'], st['sd_ready'], st['link_lines']), ('usb3', True, 4))

    def test_version_parse(self):
        v = FirmwareVersion.parse('x rp569 CH569W firmware 1.2.30 git abc-dirty built Oct  4 2026 uid ab12')
        self.assertEqual((v.major, v.minor, v.patch, v.uid), (1, 2, 30, 'AB12'))

    def test_library_version(self):
        self.assertRegex(rp569.__version__, r'^\d+\.\d+\.\d+$')


if __name__ == '__main__':
    unittest.main()
