"""Build hardware/lib/rp569.kicad_sym: CH569W symbol from phantomdrive (MIT) with corrected pin types, and a
TPS2121 power mux symbol drawn from the TI datasheet (SLVSEA3F, pin functions table, VQFN-HR RUX package)."""
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


def pin(typ, x, y, ang, name, num):
    e = '(effects (font (size 1.27 1.27)))'
    return f'(pin {typ} line (at {x} {y} {ang}) (length 2.54) (name "{name}" {e}) (number "{num}" {e}))'


def prop(k, v, y, hide=False):
    h = ' (hide yes)' if hide else ''
    return f'(property "{k}" "{v}" (at 0 {y} 0) (effects (font (size 1.27 1.27)){h}))'


def tps2121():
    left = [('power_in', 'IN1', 7), ('power_in', 'IN2', 2), ('input', 'OV1', 5), ('input', 'OV2', 4),
            ('input', 'PR1', 6), ('input', 'CP2', 3)]
    # ST is open-drain; typed passive because this board ties it to GND (datasheet: 'Connect to GND if not required')
    right = [('power_out', 'OUT', 1), ('passive', 'OUT', 8), ('passive', 'ST', 9), ('passive', 'ILIM', 10),
             ('passive', 'SS', 11)]
    pins = [pin(t, -10.16, round(6.35 - 2.54 * i, 2), 0, n, k) for i, (t, n, k) in enumerate(left)]
    pins += [pin(t, 10.16, round(6.35 - 2.54 * i, 2), 180, n, k) for i, (t, n, k) in enumerate(right)]
    pins.append(pin('power_in', 0, -11.43, 90, 'GND', 12))
    txt = ('(symbol "TPS2121" (pin_names (offset 1.016)) (exclude_from_sim no) (in_bom yes) (on_board yes) '
           + prop('Reference', 'U', 10.16) + prop('Value', 'TPS2121', -13.97)
           + prop('Footprint', 'rp569:TI_RUX0012A_VQFN-HR-12_2x2.5mm', -16.51, True)
           + prop('Datasheet', 'https://www.ti.com/lit/ds/symlink/tps2121.pdf', -19.05, True)
           + '(symbol "TPS2121_0_1" (rectangle (start -7.62 8.89) (end 7.62 -8.89) '
             '(stroke (width 0.254) (type default)) (fill (type background))))'
           + '(symbol "TPS2121_1_1" ' + ' '.join(pins) + '))')
    return parse(txt)


ver = find(parse(open(SRC).read()), 'version')[0]
os.makedirs(os.path.join(HW, 'lib'), exist_ok=True)
with open(os.path.join(HW, 'lib', 'rp569.kicad_sym'), 'w') as f:
    f.write(dump(['kicad_symbol_lib', ver, ['generator', '"rp569_gen"'], s, tps2121()]) + '\n')
