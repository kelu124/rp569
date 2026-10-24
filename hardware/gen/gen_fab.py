"""JLCPCB fabrication outputs from the routed board: hardware/jlcpcb/

    python3 hardware/gen/gen_fab.py          (system or any python3: only kicad-cli is used, plus gen_bom.py)

1. DRC gate on hardware/rp569-routed.kicad_pcb against its project rules (zones refilled): stops unless there are
   0 unconnected items and 0 errors (warnings, e.g. silkscreen, are listed but allowed).
2. Gerbers (4 copper layers, paste, silkscreen, mask, Edge.Cuts) and Excellon drill files with the settings of
   JLCPCB's KiCad guide: Protel extensions, no X2/netlist attributes, soldermask subtracted from silkscreen,
   mm / decimal / absolute origin, alternate oval-hole mode, PTH and NPTH files. Zipped as rp569-gerbers.zip.
3. BOM + CPL through gen_bom.py (CPL taken from the routed board).
"""
import collections
import glob
import json
import os
import shutil
import subprocess
import sys
import zipfile

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PCB = os.path.join(HW, 'rp569-routed.kicad_pcb')
OUT = os.path.join(HW, 'jlcpcb')
GERBER_DIR = os.path.join(OUT, 'gerbers')
LAYERS = 'F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts'


def cli(*args):
    r = subprocess.run(['kicad-cli', *args], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f'kicad-cli {" ".join(args[:3])} failed:\n{r.stdout}{r.stderr}')
    return r


def drc_gate():
    report = os.path.join(HW, 'reports', 'drc-routed.json')
    cli('pcb', 'drc', '--refill-zones', '--format', 'json', '-o', report, PCB)
    d = json.load(open(report))
    errors = [v for v in d['violations'] if v['severity'] == 'error']
    warnings = collections.Counter(v['type'] for v in d['violations'] if v['severity'] != 'error')
    print(f'DRC: {len(d["unconnected_items"])} unconnected, {len(errors)} errors, warnings {dict(warnings)}')
    if d['unconnected_items'] or errors:
        sys.exit(f'DRC not clean, see {report}: no fabrication output written')


def main():
    if not os.path.exists(PCB):
        sys.exit(f'{PCB} missing: route the board first (docs/layout.md)')
    drc_gate()
    shutil.rmtree(GERBER_DIR, ignore_errors=True)
    os.makedirs(GERBER_DIR)
    cli('pcb', 'export', 'gerbers', '--check-zones', '-l', LAYERS, '--subtract-soldermask', '--no-x2',
        '--no-netlist', '-o', GERBER_DIR + '/', PCB)
    cli('pcb', 'export', 'drill', '--format', 'excellon', '--excellon-units', 'mm', '--excellon-zeros-format',
        'decimal', '--drill-origin', 'absolute', '--excellon-oval-format', 'alternate', '--excellon-separate-th',
        '--generate-map', '--map-format', 'gerberx2', '-o', GERBER_DIR + '/', PCB)
    files = sorted(glob.glob(os.path.join(GERBER_DIR, '*')))
    zpath = os.path.join(OUT, 'rp569-gerbers.zip')
    with zipfile.ZipFile(zpath, 'w', zipfile.ZIP_DEFLATED) as z:
        for f in files:
            z.write(f, os.path.basename(f))
    print(f'{len(files)} gerber/drill files -> {os.path.relpath(zpath, HW)}')
    subprocess.run([sys.executable, os.path.join(HW, 'gen', 'gen_bom.py')], check=True)


if __name__ == '__main__':
    main()
