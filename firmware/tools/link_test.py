"""Host-side test of the RP2350 <-> CH569 UART link through the RP2350 command line.

    python3 firmware/tools/link_test.py [/dev/ttyACM0] [-n 20]

Opens the RP2350 USB serial port, sends `ch send ping <i>` n times and checks that each line comes back as
`CH> ping <i>` (the CH569 firmware echoes every byte it receives from the RP2350). Needs pyserial.
Exit code 0 when every ping came back.
"""
import argparse
import sys
import time

import serial


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('port', nargs='?', default='/dev/ttyACM0', help='RP2350 USB serial port')
    ap.add_argument('-n', type=int, default=20, help='number of pings')
    args = ap.parse_args()
    with serial.Serial(args.port, 115200, timeout=0.5) as s:
        s.write(b'\r')
        time.sleep(0.2)
        s.reset_input_buffer()
        s.write(b'ch status\r')
        print(s.read(400).decode(errors='replace').strip())
        ok = 0
        for i in range(args.n):
            s.write(f'ch send ping {i}\r'.encode())
            deadline = time.time() + 1.0
            buf = ''
            while time.time() < deadline and f'CH> ping {i}' not in buf:
                buf += s.read(256).decode(errors='replace')
            if f'CH> ping {i}' in buf:
                ok += 1
            else:
                print(f'ping {i}: no echo')
        print(f'{ok}/{args.n} pings echoed by the CH569')
        sys.exit(0 if ok == args.n else 1)


if __name__ == '__main__':
    main()
