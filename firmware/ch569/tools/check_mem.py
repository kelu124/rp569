#!/usr/bin/env python3
"""Fail the build if the CH569 image would make the flasher shrink RAMX below what the firmware uses.

The CH569 shares 128 KB of SRAM between the zero-wait code ROM and RAMX (config bits USER_MEM, CH569DS1 §2,
config word bits [31:30]). wch-ch56x-isp sets USER_MEM from the image size at every flash
(cmd_set_flash_size): image < 32 KB -> ROM 32 KB + RAMX 96 KB, < 64 KB -> ROM 64 + RAMX 64, < 96 KB -> ROM 96 +
RAMX 32. sdk/Ld/Link.ld always declares RAMX as 96 KB, so the linker cannot catch an overflow: a bigger image
would silently put the .DMADATA buffers past the end of RAMX. This script reads the ELF section headers (no
toolchain needed), measures RAMX use and checks it against the RAMX that this image size leaves.

Usage: check_mem.py build/rp569_ch569.elf build/rp569_ch569.bin
"""
import os
import struct
import sys

KB = 1024
RAMX_BASE = 0x20020000
RAMX_MAX = 96 * KB
# (image size limit, RAMX left) in wch-ch56x-isp's order; the image must be strictly below the limit.
SPLITS = [(32 * KB, 96 * KB), (64 * KB, 64 * KB), (96 * KB, 32 * KB)]
# wch-ch56x-isp 0.3.1 devices.h: "#define SZ_96K (64 * SZ_1K)", so it refuses images >= 64 KB.
ISP_REAL_LIMIT = 64 * KB
SHF_ALLOC = 0x2


def sections(path):
    """(name, addr, size, flags) of every section of a 32-bit little-endian ELF."""
    data = open(path, 'rb').read()
    if data[:4] != b'\x7fELF' or data[4] != 1 or data[5] != 1:
        sys.exit(f'{path}: not a 32-bit little-endian ELF')
    shoff, = struct.unpack_from('<I', data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', data, 0x2E)
    hdrs = [struct.unpack_from('<IIIIIIIIII', data, shoff + i * shentsize) for i in range(shnum)]
    stroff = hdrs[shstrndx][4]
    out = []
    for name_off, _type, flags, addr, _off, size, *_ in hdrs:
        end = data.index(b'\0', stroff + name_off)
        out.append((data[stroff + name_off:end].decode(), addr, size, flags))
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    elf, binfile = sys.argv[1:]
    image = os.path.getsize(binfile)
    ramx = [(n, a, s) for n, a, s, f in sections(elf)
            if f & SHF_ALLOC and s and RAMX_BASE <= a < RAMX_BASE + RAMX_MAX]
    used = max((a + s for _, a, s in ramx), default=RAMX_BASE) - RAMX_BASE
    names = ', '.join(f'{n} {s / KB:.1f} KB' for n, _, s in ramx) or 'nothing'

    i = next((i for i, (limit, _) in enumerate(SPLITS) if image < limit), None)
    if i is None:
        sys.exit(f'check_mem: FAIL image {image / KB:.1f} KB: the CH569 runs at most 96 KB of code (USER_MEM)')
    limit, left = SPLITS[i]
    print(f'check_mem: image {image / KB:.1f} KB -> flasher sets ROM {(128 * KB - left) // KB} KB + '
          f'RAMX {left // KB} KB; RAMX used {used / KB:.1f} KB ({names})')
    if used > left:
        sys.exit(f'check_mem: FAIL RAMX needs {used / KB:.1f} KB but an image of {image / KB:.1f} KB leaves '
                 f'{left // KB} KB: shrink .DMADATA (DMA buffers) or the code')
    if image >= ISP_REAL_LIMIT:
        print('check_mem: warning: wch-ch56x-isp 0.3.1 refuses images >= 64 KB (SZ_96K is 64 KB in devices.h)')
    margin = f'{(limit - image) / KB:.1f} KB of code left before the flasher'
    if i + 1 < len(SPLITS):
        nxt = SPLITS[i + 1][1]
        margin += f' cuts RAMX to {nxt // KB} KB' + (' (this RAMX use would then FAIL)' if used > nxt else '')
    print(f'check_mem: OK, {(left - used) / KB:.1f} KB RAMX spare, {margin}')

if __name__ == '__main__':
    main()
