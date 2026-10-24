"""Hardware test of the RP2350 <-> CH569 UART link, through the rp569 python library.

    PYTHONPATH=python python3 firmware/tools/link_test.py [-n 20] [--serial <RP2350 uid>]

Finds the RP2350 command line by VID/PID/product, prints both firmware versions, sends `ch send ping <i>` n times
and checks that the CH569 echoes each line back. Exit code 0 when every ping came back.
"""
import argparse
import sys

import rp569


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-n', type=int, default=20, help='number of pings')
    ap.add_argument('--serial', help='RP2350 unique ID, when several boards are plugged in')
    args = ap.parse_args()
    print(f'rp569 python library {rp569.__version__}')
    with rp569.RP2350(serial_number=args.serial) as rp:
        v = rp.version()
        print(f'RP2350B firmware {v.version} (git {v.git}) uid {v.uid} on {rp.port}')
        st = rp.ch_status()
        print(f'CH569 power {"on" if st["power"] else "off"}, 3V3_CH {st["3V3_CH"]:.2f} V, VBUS3 {st["VBUS3"]:.2f} V')
        if not st['power']:
            rp.ch_on()
        try:
            with rp569.CH569Console() as con:
                c = con.version()
                print(f'CH569W firmware {c.version} (git {c.git}) uid {c.uid} on {con.port}')
        except rp569.PortNotFound as e:
            print(f'CH569 console not found ({e}); testing the echo only')
        ok = sum(rp.ch_send(f'ping {i}') == f'ping {i}' for i in range(args.n))
        print(f'{ok}/{args.n} pings echoed by the CH569')
        sys.exit(0 if ok == args.n else 1)


if __name__ == '__main__':
    main()
