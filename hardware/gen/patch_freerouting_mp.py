"""Patch Freerouting 2.1.0 BatchAutorouter: the stop check reads RouterSettings.maxPasses (-mp) instead of
get_stop_pass_no(), which the CLI never sets (stock 2.1.0 runs until its 12 h job timeout).

    cd $(mktemp -d) && unzip -q ~/.local/share/freerouting/freerouting-2.1.0.jar app/freerouting/autoroute/BatchAutorouter.class
    python3 patch_freerouting_mp.py app/freerouting/autoroute/BatchAutorouter.class
    cp ~/.local/share/freerouting/freerouting-2.1.0.jar ~/.local/share/freerouting/freerouting-2.1.0-mp.jar
    zip -q ~/.local/share/freerouting/freerouting-2.1.0-mp.jar app/freerouting/autoroute/BatchAutorouter.class

Constant #457 (Methodref get_stop_pass_no:()I) becomes a Fieldref maxPasses:I and its invokevirtual a getfield
(same length and stack effect).
"""
import struct, sys
p = sys.argv[1]
b = bytearray(open(p, 'rb').read())
n = struct.unpack('>H', b[8:10])[0]
pos, cp = 10, [None]
i = 1
while i < n:
    tag = b[pos]
    start = pos
    if tag == 1:
        ln = struct.unpack('>H', b[pos + 1:pos + 3])[0]; val = bytes(b[pos + 3:pos + 3 + ln]).decode('utf8', 'replace'); pos += 3 + ln
        cp.append((tag, start, val))
    elif tag in (3, 4):
        cp.append((tag, start, None)); pos += 5
    elif tag in (5, 6):
        cp.append((tag, start, None)); cp.append(None); pos += 9; i += 1
    elif tag in (7, 8, 16, 19, 20):
        cp.append((tag, start, struct.unpack('>H', b[pos + 1:pos + 3])[0])); pos += 3
    elif tag in (9, 10, 11, 12, 17, 18):
        cp.append((tag, start, struct.unpack('>HH', b[pos + 1:pos + 5]))); pos += 5
    elif tag == 15:
        cp.append((tag, start, None)); pos += 4
    else:
        raise SystemExit(f'unknown tag {tag}')
    i += 1
utf = {e[2]: k for k, e in enumerate(cp) if e and e[0] == 1}
mref = 457
tag, start, (cls, nat) = cp[mref]
ntag, nstart, (name_i, desc_i) = cp[nat]
assert tag == 10 and cp[name_i][2] == 'get_stop_pass_no', (cp[mref], cp[name_i])
assert 'I' in utf
if 'maxPasses' not in utf:                                       # append a Utf8 entry; existing indices stay valid
    entry = bytes([1]) + struct.pack('>H', 9) + b'maxPasses'
    b[pos:pos] = entry
    utf['maxPasses'] = n
    b[8:10] = struct.pack('>H', n + 1)
# reuse the NameAndType slot: name -> maxPasses, descriptor -> I (only used by this Methodref?)
users = [k for k, e in enumerate(cp) if e and e[0] in (9, 10, 11) and e[2][1] == nat]
assert users == [mref], users
b[start] = 9                                                     # Methodref -> Fieldref
b[nstart + 1:nstart + 5] = struct.pack('>HH', utf['maxPasses'], utf['I'])
call = bytes([0xb6]) + struct.pack('>H', mref)
assert b.count(call) == 1, b.count(call)
k = b.find(call); b[k] = 0xb4                                    # invokevirtual -> getfield
open(p, 'wb').write(b)
print('patched')
