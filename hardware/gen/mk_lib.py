"""Build hardware/lib/rp569.kicad_sym: CH569W symbol from phantomdrive (MIT) with corrected pin types."""
import os
from kisym import dump, find, get_symbol, parse, unq

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(HW, '..', 'project', 'phantomdrive', 'ee', 'lib', 'ch565w.kicad_sym')
POWER = {'V33USB', 'V12USB', 'V12CORE', 'VDDIO', 'V33LDO', 'V33EX', 'EP'}

s = get_symbol(SRC, 'ch569w')
s[1] = '"CH569W"'
for sub in find(s, 'symbol'):
    sub[1] = sub[1].replace('ch569w', 'CH569W')
    for p in find(sub, 'pin'):
        name = unq(find(p, 'name')[0][1])
        p[1] = 'power_in' if name in POWER else 'output' if name == 'XO' else 'input' if name == 'XI' else 'bidirectional'
for p in find(s, 'property'):
    if unq(p[1]) == 'Value':
        p[2] = '"CH569W"'
    if unq(p[1]) == 'Datasheet':
        p[2] = '"https://www.wch-ic.com/products/CH569.html"'
ver = find(parse(open(SRC).read()), 'version')[0]
os.makedirs(os.path.join(HW, 'lib'), exist_ok=True)
with open(os.path.join(HW, 'lib', 'rp569.kicad_sym'), 'w') as f:
    f.write(dump(['kicad_symbol_lib', ver, ['generator', '"rp569_gen"'], s]) + '\n')
