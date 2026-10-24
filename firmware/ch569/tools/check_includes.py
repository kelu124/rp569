#!/usr/bin/env python3
"""Catch case-mismatched #include "..." against actual filenames.

Windows/macOS hide these (case-insensitive FS); a case-sensitive Linux build
(or a clean checkout on the client's machine) fails on them. Run in CI so the
class of bug can never ship again.
"""
import os, re, sys

hdrs = {}
for root, _, files in os.walk('.'):
    if '/build' in root.replace('\\', '/') or '/.git' in root.replace('\\', '/'):
        continue
    for f in files:
        if f.lower().endswith(('.h', '.inc')):
            hdrs.setdefault(f.lower(), set()).add(f)

inc_re = re.compile(r'#\s*include\s*"([^"]+)"')
bad, seen = [], set()
for root, _, files in os.walk('.'):
    if '/build' in root.replace('\\', '/') or '/.git' in root.replace('\\', '/'):
        continue
    for f in files:
        if not f.lower().endswith(('.c', '.h', '.s')):
            continue
        p = os.path.join(root, f)
        try:
            txt = open(p, encoding='utf-8', errors='replace').read()
        except OSError:
            continue
        for m in inc_re.finditer(txt):
            inc = m.group(1).split('/')[-1]
            real = hdrs.get(inc.lower())
            if real is None or inc in real:
                continue
            k = (p, inc)
            if k in seen:
                continue
            seen.add(k)
            bad.append((p.replace('\\', '/'), inc, sorted(real)))

for p, inc, real in bad:
    print(f'  [FAIL] {p}: includes "{inc}" but file is {real}')

if bad:
    print(f"INCLUDE CASE CHECK: {len(bad)} mismatch(es)")
    sys.exit(1)
print("INCLUDE CASE CHECK: all includes match real filenames")
