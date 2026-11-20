"""Host side of firmware variant 1 ("stream", D-031): read the CH569 bulk stream and check the counter pattern.

The RP2350 sends 32-bit little-endian counter words (continuing across packets) on the 8-bit HSPI bus, the CH569
forwards them on its vendor bulk IN endpoint 0x82 (1a86:fe2c, product "rp569 CH569 stream"). Needs pyusb + libusb
and USB access to 1a86:fe2c (udev rule, docs/toolchain.md); numpy for the check at full rate.

    with StreamReader() as s:
        res = s.measure(seconds=5)       # MB/s, words checked, gaps, CH569 statistics
"""
import struct
import time

import numpy as np
import usb.core
import usb.util

VID, PID = 0x1A86, 0xFE2C
EP_IN = 0x82
REQ_STATS, REQ_RESET, REQ_EDGE = 0x60, 0x61, 0x62
STATS_FIELDS = ('packets', 'bytes_in', 'crc_errors', 'seq_errors', 'overflows', 'fifo_ov', 'bytes_out',
                'usb_packets', 'loop_max_us')     # loop_max_us: longest CH569 main-loop pass (stream fw >= 1.0.x)


class StreamError(RuntimeError):
    pass


def find_device(serial: str | None = None):
    for dev in usb.core.find(find_all=True, idVendor=VID, idProduct=PID):
        try:
            product = usb.util.get_string(dev, dev.iProduct) or ''
            sn = usb.util.get_string(dev, dev.iSerialNumber) or ''
        except (usb.core.USBError, ValueError) as e:
            raise StreamError(f'cannot read the CH569 strings ({e}): USB access? (udev rule for 1a86:fe2c)') from e
        if 'stream' in product and (serial is None or sn.upper() == serial.upper()):
            return dev
    raise StreamError('no CH569 in stream mode (product "rp569 CH569 stream"): flash the stream firmware')


def _reader_process(serial, chunk, timeout_ms, q, stop):
    """Child process: open the CH569 and keep bulk reads going; puts (bytes, t) items, None at the end."""
    dev = find_device(serial)
    dev.set_configuration()
    usb.util.claim_interface(dev, 0)
    q.put(True)
    got = False
    while not stop.is_set():
        try:
            data = dev.read(EP_IN, chunk, timeout=timeout_ms)
        except usb.core.USBTimeoutError:
            if got:
                break
            continue
        got = True
        q.put((bytes(data), time.perf_counter()))
    q.put(None)
    usb.util.release_interface(dev, 0)
    usb.util.dispose_resources(dev)


class StreamReader:
    def __init__(self, serial: str | None = None, chunk: int = 512 << 10):
        self.serial = serial
        # bytes per bulk read (multiple of 512). Board #1 at 40 MB/s (sync reads, CH569 overflows in 8 s): 64 kB 9846,
        # 128 kB 2327, 256 kB 32, 512 kB 8, 1 MB 8662, 4 MB 2390: 512 kB.
        self.chunk = chunk
        self._open()

    def _open(self):
        self.dev = find_device(self.serial)
        try:
            if self.dev.is_kernel_driver_active(0):
                self.dev.detach_kernel_driver(0)
        except (NotImplementedError, usb.core.USBError):
            pass
        self.dev.set_configuration()
        usb.util.claim_interface(self.dev, 0)

    def close(self):
        usb.util.release_interface(self.dev, 0)
        usb.util.dispose_resources(self.dev)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    # -- control
    def stats(self) -> dict:
        raw = self.dev.ctrl_transfer(0xC0, REQ_STATS, 0, 0, 4 * len(STATS_FIELDS))
        return dict(zip(STATS_FIELDS, struct.unpack('<%dI' % len(STATS_FIELDS), bytes(raw))))

    def reset(self):
        self.dev.ctrl_transfer(0x40, REQ_RESET, 0, 0, None)

    def set_edge(self, falling: bool):
        self.dev.ctrl_transfer(0x40, REQ_EDGE, 1 if falling else 0, 0, None)

    # -- data
    def drain(self, timeout_ms: int = 200):
        """Read and drop whatever is queued (before a measurement)."""
        while True:
            try:
                self.dev.read(EP_IN, self.chunk, timeout=timeout_ms)
            except usb.core.USBTimeoutError:
                return

    def measure(self, seconds: float = 5.0, timeout_ms: int = 1000, start=None) -> dict:
        """Read for `seconds` and check that every 32-bit word is the previous one + 1. A reader process only
        re-issues bulk reads (the CH569 ring holds ~5 ms at 16 MB/s: any pause in the IN requests overflows it), the
        check runs here from a queue. `start` (optional) starts the source once the reads are flowing. Returns MB/s (1e6 B/s), bytes, words, the first word seen,
        counter gaps, words lost, and the time to the first byte."""
        import multiprocessing as mp
        import queue
        # The reader runs in its own process: in a thread it waited for the GIL while numpy checked a chunk, and the
        # CH569 ring (~5 ms at 16 MB/s) overflowed (board #1: 1 gap in 10 s). It only re-issues bulk reads.
        ctx = mp.get_context('fork')
        q = ctx.Queue(maxsize=256)
        stop = ctx.Event()
        t0 = time.perf_counter()
        times = {}
        self.close()                          # the child opens the device itself
        th = ctx.Process(target=_reader_process, args=(self.serial, self.chunk, timeout_ms, q, stop), daemon=True)
        th.start()
        if not q.get(timeout=10):             # the child reports once its first read is queued
            raise StreamError('stream reader process could not open the CH569')
        if start is not None:            # start the source only once IN requests are flowing
            time.sleep(0.05)
            start()
            end_shift = time.perf_counter() - t0
        else:
            end_shift = 0
        total = gaps = lost = 0
        gap_at = []                           # (word index, words lost) of the first gaps
        first = expect = None
        tail = b''
        end = t0 + end_shift + seconds
        while True:
            if time.perf_counter() >= end:
                stop.set()
            try:
                data = q.get(timeout=0.5)
            except queue.Empty:
                if stop.is_set() and not th.is_alive():
                    break
                continue
            if data is None:
                break
            data, t_rx = data
            times.setdefault('first', t_rx)
            times['last'] = t_rx
            data = tail + data
            total += len(data) - len(tail)
            n = len(data) // 4
            tail = data[n * 4:]
            w = np.frombuffer(data[:n * 4], dtype='<u4')
            if not len(w):
                continue
            if expect is None:
                first = expect = int(w[0])
            if int(w[0]) != expect:
                gaps += 1
                lost += (int(w[0]) - expect) & 0xFFFFFFFF
                if len(gap_at) < 8:
                    gap_at.append((expect, (int(w[0]) - expect) & 0xFFFFFFFF, round(t_rx - t0, 3)))
            d = np.diff(w.astype(np.int64))
            jumps = np.nonzero(d != 1)[0]
            gaps += len(jumps)
            if len(jumps):
                lost += int((d[jumps] - 1).sum())
                for j in jumps[:max(0, 8 - len(gap_at))]:
                    gap_at.append((int(w[j]) + 1, int(d[j]) - 1, round(t_rx - t0, 3)))
            expect = (int(w[-1]) + 1) & 0xFFFFFFFF
        th.join(timeout=3)
        self._open()
        start = times.get('first', t0)
        secs = max(times.get('last', start) - start, 1e-9)
        return {'mbs': total / secs / 1e6, 'bytes': total, 'seconds': secs, 'words': total // 4,
                'first_word': first, 'gaps': gaps, 'words_lost': lost, 'gap_at': gap_at,
                'first_byte_s': (times['first'] - t0) if 'first' in times else None}
