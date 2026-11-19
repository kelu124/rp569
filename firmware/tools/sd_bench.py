"""SD card benchmark through the CH569 drive, logged against the firmware version and the card's CID.

    PYTHONPATH=python python3 firmware/tools/sd_bench.py [--mb 64] [--runs 1] [--note "..."]
    PYTHONPATH=python python3 firmware/tools/sd_bench.py --show

1. CH569 to serial mode (host unmount + `ch eject`), read its firmware version, USB speed and the card's CID / size /
   bus clock from the console (`version`, `status`, `sd`).
2. Console `mount`: back to drive mode, wait for the host to mount the card.
3. Per run: write --mb MB of numbered 512-byte sectors to a file + fsync (write speed), unmount + mount (no page
   cache), read it back (read speed), check every sector, delete the file.
4. One row per run appended to firmware/ch569/sd_benchmarks.csv (written with a header if new).

Exit code 0 when every run verified. Needs Linux, udisksctl, the CH569 firmware with the `sd` console command.
"""
import argparse
import csv
import datetime
import os
import struct
import subprocess
import sys
import time

import rp569
from rp569 import drive, ports

LOG = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'ch569', 'sd_benchmarks.csv')
FIELDS = ['date', 'ch569_fw', 'ch569_git', 'rp2350_fw', 'usb', 'card_maker', 'card_mid', 'card_oid', 'card_name',
          'card_rev', 'card_serial', 'card_date', 'card_mb', 'sd_clock_mhz', 'file_mb', 'write_mbs', 'read_mbs',
          'bad_sectors', 'note', 'card_cid']
SEC = 512

# SD Association manufacturer IDs seen in the wild (not an official public list)
MAKERS = {0x01: 'Panasonic', 0x02: 'Toshiba/Kioxia', 0x03: 'SanDisk', 0x12: 'Patriot', 0x1B: 'Samsung',
          0x1D: 'ADATA', 0x27: 'Phison', 0x28: 'Lexar', 0x31: 'Silicon Power', 0x41: 'Kingston', 0x6F: 'STMicro/Netac',
          0x74: 'Transcend', 0x76: 'Patriot', 0x82: 'Sony', 0x9C: 'Angelbird/Hoodman', 0x9F: 'Kingston/Taishin'}


def wait_for(cond, timeout, what):
    end = time.time() + timeout
    while time.time() < end:
        v = cond()
        if v:
            return v
        time.sleep(0.3)
    sys.exit(f'timeout waiting for {what}')


def mounted_partition():
    for dev, mnt in drive.partitions():
        if mnt:
            return dev, mnt
    return None


def mount_drive():
    """The CH569 drive's partition, mounted (udisks usually automounts it; mount it if not)."""
    parts = wait_for(lambda: drive.partitions(), 20, 'the CH569 drive')
    m = mounted_partition()
    if not m:
        subprocess.run(['udisksctl', 'mount', '-b', parts[0][0]], check=True, capture_output=True)
        m = wait_for(mounted_partition, 10, 'the mount')
    return m


def card_identity(rp):
    """Serial mode -> versions + card info from the console -> drive mode again."""
    if ports.ch569_usb_mode() == 'drive':
        drive.eject(rp)
    wait_for(lambda: ports.ch569_usb_mode() == 'serial', 15, 'CH569 serial mode')
    con_port = wait_for(lambda: next((p for p in ports.list_ports_rp569() if p.role == 'ch569-console'), None),
                        15, 'the CH569 console port')
    time.sleep(0.5)
    with rp569.CH569Console(con_port.device) as con:
        v = con.version()
        st = con.status()
        sd = con.sd_info()
        con.mount()
    if sd is None:
        sys.exit('the CH569 has no card mounted (sd none)')
    wait_for(lambda: ports.ch569_usb_mode() == 'drive', 15, 'CH569 drive mode')
    return v, st, sd


def pattern(n):
    out = bytearray()
    for i in range(n):
        out += (struct.pack('<I', i) + b'RP569SEC%08d' % i).ljust(SEC, bytes([i & 0xFF]))
    return bytes(out)


def run_once(mb):
    dev, mnt = mount_drive()
    n = mb * 2048
    data = pattern(n)
    path = os.path.join(mnt, 'rp569_bench.bin')
    t0 = time.perf_counter()
    with open(path, 'wb') as f:
        for i in range(0, len(data), 1 << 20):
            f.write(data[i:i + (1 << 20)])
        f.flush()
        os.fsync(f.fileno())
    t1 = time.perf_counter()
    subprocess.run(['udisksctl', 'unmount', '-b', dev], check=True, capture_output=True)
    subprocess.run(['udisksctl', 'mount', '-b', dev], check=True, capture_output=True)
    dev, mnt = wait_for(mounted_partition, 10, 'the remount')
    path = os.path.join(mnt, 'rp569_bench.bin')
    t2 = time.perf_counter()
    with open(path, 'rb') as f:
        back = f.read()
    t3 = time.perf_counter()
    bad = sum(1 for i in range(n) if back[i * SEC:(i + 1) * SEC] != data[i * SEC:(i + 1) * SEC]) \
        if len(back) == len(data) else n
    os.remove(path)
    os.sync()
    return mb / (t1 - t0), mb / (t3 - t2), bad


def show():
    if not os.path.exists(LOG):
        sys.exit(f'no log yet ({LOG})')
    with open(LOG) as f:
        rows = list(csv.DictReader(f))
    cols = ['date', 'ch569_fw', 'usb', 'card_maker', 'card_name', 'card_serial', 'card_mb', 'sd_clock_mhz',
            'file_mb', 'write_mbs', 'read_mbs', 'bad_sectors', 'note']
    width = {c: max(len(c), *(len(r.get(c, '')) for r in rows)) for c in cols}
    print('  '.join(c.ljust(width[c]) for c in cols))
    for r in rows:
        print('  '.join(r.get(c, '').ljust(width[c]) for c in cols))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--mb', type=int, default=64, help='file size per run (MB)')
    ap.add_argument('--runs', type=int, default=1)
    ap.add_argument('--note', default='')
    ap.add_argument('--show', action='store_true', help='print the log and exit')
    args = ap.parse_args()
    if args.show:
        return show()

    print(f'rp569 python library {rp569.__version__}')
    with rp569.RP2350() as rp:
        rv = rp.version()
        if not rp.ch_status()['power']:
            rp.ch_on()
        v, st, sd = card_identity(rp)
    maker = MAKERS.get(sd['mid'], 'unknown')
    print(f"CH569W {v.version} (git {v.git}), {st['usb']}; card {maker} (0x{sd['mid']:02X}) '{sd['name']}' rev "
          f"{sd['rev']} serial {sd['serial']} made {sd['date']}, {sd['size_mb']} MB, bus {sd['clock_mhz']} MHz")

    new = not os.path.exists(LOG)
    ok = True
    with open(LOG, 'a', newline='') as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        if new:
            w.writeheader()
        for i in range(args.runs):
            wr, rd, bad = run_once(args.mb)
            ok &= bad == 0
            print(f'run {i + 1}: {args.mb} MB  write {wr:.2f} MB/s  read {rd:.2f} MB/s  bad sectors {bad}')
            w.writerow({'date': datetime.datetime.now().isoformat(timespec='seconds'), 'ch569_fw': v.version,
                        'ch569_git': v.git, 'rp2350_fw': rv.version, 'usb': st['usb'], 'card_maker': maker,
                        'card_mid': f"0x{sd['mid']:02X}", 'card_oid': sd['oid'], 'card_name': sd['name'],
                        'card_rev': sd['rev'], 'card_serial': sd['serial'], 'card_date': sd['date'],
                        'card_mb': sd['size_mb'], 'sd_clock_mhz': sd['clock_mhz'], 'file_mb': args.mb,
                        'write_mbs': f'{wr:.2f}', 'read_mbs': f'{rd:.2f}', 'bad_sectors': bad,
                        'note': args.note, 'card_cid': sd['cid']})
            f.flush()
    print(f'logged to {os.path.relpath(LOG)}')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
