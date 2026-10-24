"""JLCPCB assembly files: hardware/jlcpcb/rp569-bom.csv and rp569-cpl.csv.

BOM rows are grouped by LCSC part (or by MPN for consigned parts). The CPL comes from the routed board
(`rp569-routed.kicad_pcb`, whose part positions can differ slightly from the generated `rp569.kicad_pcb`: the
autorouter's decap cleanup nudges caps) through `kicad-cli pcb export pos`, or from the generated board if there is
no routed one. JLCPCB's rotation convention differs from KiCad's for some packages: check every
part in JLCPCB's placement preview before ordering.
"""
import csv
import os
import subprocess
from collections import OrderedDict

import board

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(HW, 'jlcpcb')
ROUTED = os.path.join(HW, 'rp569-routed.kicad_pcb')
PCB = ROUTED if os.path.exists(ROUTED) else os.path.join(HW, 'rp569.kicad_pcb')


def main():
    os.makedirs(OUT, exist_ok=True)
    groups = OrderedDict()
    for p in board.parts:
        if 'mpn' not in p or p['dnp']:
            continue
        groups.setdefault((p['lcsc'] or p['mpn'], p['fp']), []).append(p)
    with open(os.path.join(OUT, 'rp569-bom.csv'), 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['Comment', 'Designator', 'Footprint', 'LCSC Part #', 'MPN', 'Qty', 'Note'])
        for ps in groups.values():
            refs = sorted((x['ref'] for x in ps), key=lambda r: (r.rstrip('0123456789'), int(r[len(r.rstrip('0123456789')):])))
            p = ps[0]
            value = p['value'] if len({x['value'] for x in ps}) == 1 else p['mpn']
            note = 'consigned (not stocked by JLCPCB)' if not p['lcsc'] else ('THT' if p['fp'].startswith('Connector_PinHeader') else '')
            w.writerow([value, ','.join(refs), p['fp'].split(':')[1], p['lcsc'], p['mpn'], len(refs), note])
    pos = os.path.join(OUT, 'pos.csv')
    subprocess.run(['kicad-cli', 'pcb', 'export', 'pos', '--format', 'csv', '--units', 'mm', '--side', 'both',
                    '--exclude-dnp', '-o', pos, PCB], check=True, capture_output=True)
    ok = {p['ref'] for p in board.parts if 'mpn' in p and not p['dnp']}
    with open(pos) as fi, open(os.path.join(OUT, 'rp569-cpl.csv'), 'w', newline='') as fo:
        w = csv.writer(fo)
        w.writerow(['Designator', 'Mid X', 'Mid Y', 'Layer', 'Rotation'])
        for r in csv.DictReader(fi):
            if r['Ref'] in ok:
                w.writerow([r['Ref'], r['PosX'] + 'mm', r['PosY'] + 'mm', 'Top' if r['Side'] == 'top' else 'Bottom',
                            r['Rot']])
    os.remove(pos)
    print(f'{len(groups)} BOM lines, {len(ok)} placed parts (CPL from {os.path.basename(PCB)}) -> {OUT}')


if __name__ == '__main__':
    main()
