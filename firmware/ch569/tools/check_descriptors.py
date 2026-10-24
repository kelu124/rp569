#!/usr/bin/env python3
"""Static USB-descriptor consistency check for the CH569 SD/UART bridge.

Parses the descriptor byte arrays out of the C sources and validates their
structure without any hardware:
  - device descriptor: length, class triplet (EF/02/01 = composite/IAD)
  - config descriptor: wTotalLength vs actual bytes, bNumInterfaces vs
    interface descriptors found, descriptor bLength chain integrity
  - every SuperSpeed endpoint descriptor is followed by a companion (0x30)
  - endpoint address/type map matches the design (EP1 notify, EP2/3 MSC,
    EP4 CDC), with sane wMaxPacketSize per speed
  - IAD groups the two CDC interfaces

Usage: check_descriptors.py <CH56x_usb30.c> <CH56x_usb20.c>
"""
import re
import sys

FAIL = 0


def err(msg):
    global FAIL
    FAIL = 1
    print(f"  [FAIL] {msg}")


def ok(msg):
    print(f"  [ ok ] {msg}")


def extract_array(text, name, defines):
    m = re.search(rf"{name}\s*\[\]\s*=\s*\{{(.*?)\}}\s*;", text, re.S)
    if not m:
        return None
    body = re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    # Minimal preprocessor: honour `#if NAME / #else / #endif` inside the array
    # so a conditional descriptor block (e.g. the debug CDC under DEBUG_OVER_USB)
    # is included or dropped per the active config.
    out, emit = [], [True]
    for line in body.splitlines():
        s = line.strip()
        if s.startswith("#if"):
            tok = s.split()[1]
            emit.append(emit[-1] and bool(defines.get(tok, 0)))
        elif s.startswith("#else"):
            emit[-1] = (not emit[-1]) and emit[-2]
        elif s.startswith("#endif"):
            emit.pop()
        elif emit[-1]:
            out.append(line)
    body = "\n".join(out)
    vals = []
    for tok in body.split(","):
        tok = tok.strip()
        if not tok:
            continue
        for name_, val in defines.items():
            tok = re.sub(rf"\b{name_}\b", str(val), tok)
        try:
            vals.append(eval(tok, {"__builtins__": {}}) & 0xFF)
        except Exception:
            raise SystemExit(f"cannot evaluate token '{tok}' in {name}")
    return bytes(vals)


def read_defines(*paths):
    defines = {}
    for p in paths:
        try:
            text = open(p, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        for m in re.finditer(r"#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)\b", text):
            defines[m.group(1)] = int(m.group(2), 0)
    return defines


def walk_config(cfg, superspeed, expected_eps):
    total = cfg[2] | (cfg[3] << 8)
    if total != len(cfg):
        err(f"wTotalLength {total} != actual {len(cfg)} bytes")
    else:
        ok(f"wTotalLength {total} matches byte count")

    n_if_declared = cfg[4]
    pos, interfaces, endpoints, iads = 0, [], [], []
    pending_companion = False
    while pos < len(cfg):
        blen, btype = cfg[pos], cfg[pos + 1]
        if blen == 0:
            err(f"zero bLength at offset {pos}")
            break
        if pending_companion and btype != 0x30:
            err(f"SS endpoint at offset {pos} lacks a companion descriptor")
        pending_companion = False

        if btype == 0x04:
            interfaces.append((cfg[pos + 2], cfg[pos + 4], cfg[pos + 5]))
        elif btype == 0x05:
            addr, attr = cfg[pos + 2], cfg[pos + 3]
            mps = cfg[pos + 4] | (cfg[pos + 5] << 8)
            endpoints.append((addr, attr & 3, mps))
            if superspeed:
                pending_companion = True
        elif btype == 0x0B:
            iads.append((cfg[pos + 2], cfg[pos + 3]))
        pos += blen
    if pos != len(cfg):
        err(f"descriptor chain overruns: pos {pos} vs len {len(cfg)}")
    else:
        ok("bLength chain walks the array exactly")

    if len(interfaces) != n_if_declared:
        err(f"bNumInterfaces {n_if_declared} but found {len(interfaces)} interface descriptors")
    else:
        ok(f"bNumInterfaces {n_if_declared} matches")

    got = {(a, t) for a, t, _ in endpoints}
    want = set(expected_eps)
    if got != want:
        err(f"endpoint map {sorted(got)} != expected {sorted(want)}")
    else:
        ok(f"endpoint map matches: {sorted(got)}")

    mps_want = 1024 if superspeed else 512
    for addr, typ, mps in endpoints:
        if typ == 2 and mps != mps_want:
            err(f"bulk EP 0x{addr:02X} wMaxPacketSize {mps}, expected {mps_want}")
    ok(f"bulk wMaxPacketSize == {mps_want}")

    return interfaces, iads


def main():
    usb30_c, usb20_c = sys.argv[1], sys.argv[2]
    here = usb30_c.rsplit("/", 1)[0] if "/" in usb30_c else "."
    defines = read_defines(f"{here}/CH56x_usb30.h", f"{here}/bridge_config.h")
    # CH56x_usb30.h defines SIZE_CONFIG_DESC / BRIDGE_NUM_INTERFACES twice under
    # #if DEBUG_OVER_USB; pin them to the active branch so the parser is right.
    debug_usb = defines.get("DEBUG_OVER_USB", 0)
    defines["SIZE_CONFIG_DESC"] = 212 if debug_usb else 128
    defines["BRIDGE_NUM_INTERFACES"] = 5 if debug_usb else 3
    t30 = open(usb30_c, encoding="utf-8", errors="replace").read()
    t20 = open(usb20_c, encoding="utf-8", errors="replace").read()

    print("== SuperSpeed device descriptor ==")
    dev = extract_array(t30, "SS_DeviceDescriptor", defines)
    if len(dev) != 18 or dev[0] != 18:
        err(f"device descriptor length {len(dev)}/bLength {dev[0]}")
    else:
        ok("18 bytes")
    if (dev[4], dev[5], dev[6]) != (0xEF, 0x02, 0x01):
        err(f"device class triplet {dev[4]:02X}/{dev[5]:02X}/{dev[6]:02X}, want EF/02/01 (IAD)")
    else:
        ok("class EF/02/01 (composite with IAD)")
    if dev[7] != 9:
        err(f"bMaxPacketSize0 {dev[7]}, want 9 (2^9=512) at SuperSpeed")
    else:
        ok("bMaxPacketSize0 = 512")

    print("== SuperSpeed configuration descriptor ==")
    cfg = extract_array(t30, "SS_ConfigDescriptor", defines)
    eps = [(0x82, 2), (0x03, 2), (0x81, 3), (0x84, 2), (0x04, 2)]
    if debug_usb:
        eps += [(0x85, 3), (0x86, 2), (0x06, 2)]   # 2nd CDC: notify + data in/out
    interfaces, iads = walk_config(cfg, superspeed=True, expected_eps=eps)

    # walk_config stores (bInterfaceNumber, bNumEndpoints, bInterfaceClass)
    by_num = {i[0]: i for i in interfaces}
    if by_num[0][2] != 0x08:
        err(f"IF0 class 0x{by_num[0][2]:02X}, want 0x08 (MSC)")
    else:
        ok("IF0 = Mass Storage")
    if by_num[1][2] != 0x02:
        err(f"IF1 class 0x{by_num[1][2]:02X}, want 0x02 (CDC comm)")
    else:
        ok("IF1 = CDC Communications")
    if by_num[2][2] != 0x0A:
        err(f"IF2 class 0x{by_num[2][2]:02X}, want 0x0A (CDC data)")
    else:
        ok("IF2 = CDC Data")

    want_iads = [(1, 2), (3, 2)] if debug_usb else [(1, 2)]
    if debug_usb:
        if by_num.get(3, (0, 0, 0))[2] != 0x02 or by_num.get(4, (0, 0, 0))[2] != 0x0A:
            err("debug CDC interfaces IF3/IF4 missing or wrong class")
        else:
            ok("IF3/IF4 = debug CDC (comm + data)")
    if iads != want_iads:
        err(f"IAD list {iads}, want {want_iads}")
    else:
        ok(f"IAD grouping correct: {iads}")

    print("== BOS descriptor ==")
    bos = extract_array(t30, "BOSDescriptor", defines)
    bos_total = bos[2] | (bos[3] << 8)
    if bos_total != len(bos):
        err(f"BOS wTotalLength {bos_total} != {len(bos)}")
    else:
        ok(f"BOS length {bos_total}")

    print("== USB2 (high-speed fallback, MSC-only) configuration ==")
    cfg20 = extract_array(t20, "hs_config_descriptor", defines)
    walk_config(cfg20, superspeed=False, expected_eps=[(0x82, 2), (0x03, 2)])

    print()
    if FAIL:
        print("DESCRIPTOR CHECK: FAILED")
        sys.exit(1)
    print("DESCRIPTOR CHECK: all good")


if __name__ == "__main__":
    main()
