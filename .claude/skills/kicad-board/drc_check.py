"""DRC a board against a given project's rules and summarise it (unconnected links with coordinates, violations).

    python3 .claude/skills/kicad-board/drc_check.py [board.kicad_pcb] [--pro hardware/rp569.kicad_pro] [--json out]

Default board: hardware/rp569-routed.kicad_pcb. The board is copied next to the chosen project file into a temp
dir first, so it is always graded against the real project rules (KRT steps write their own, laxer .kicad_pro).
Zones are refilled. Used to check the user's hand edits ("check my edits").
"""
import argparse
import collections
import json
import os
import shutil
import subprocess
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
COSMETIC = {'silk_overlap', 'silk_over_copper', 'lib_footprint_issues'}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('board', nargs='?', default=os.path.join(ROOT, 'hardware', 'rp569-routed.kicad_pcb'))
    ap.add_argument('--pro', default=os.path.join(ROOT, 'hardware', 'rp569.kicad_pro'))
    ap.add_argument('--json', help='keep the DRC report here')
    a = ap.parse_args()
    tmp = tempfile.mkdtemp()
    shutil.copy(a.board, os.path.join(tmp, 'b.kicad_pcb'))
    shutil.copy(a.pro, os.path.join(tmp, 'b.kicad_pro'))
    rep = a.json or os.path.join(tmp, 'drc.json')
    subprocess.run(['kicad-cli', 'pcb', 'drc', '--refill-zones', '--format', 'json', '-o', rep,
                    os.path.join(tmp, 'b.kicad_pcb')], check=True, capture_output=True)
    d = json.load(open(rep))
    sev = collections.Counter(v['severity'] for v in d['violations'])
    print(f"{a.board}: {len(d['unconnected_items'])} unconnected, {sev.get('error', 0)} errors, "
          f"{sev.get('warning', 0)} warnings {dict(collections.Counter(v['type'] for v in d['violations']))}")
    by_net = collections.Counter()
    for it in d['unconnected_items']:
        x, y = it['items'][0], it['items'][1]
        print(f"  U {x['description'][:50]:50} ({x['pos']['x']:.1f},{x['pos']['y']:.1f}) -> "
              f"{y['description'][:50]} ({y['pos']['x']:.1f},{y['pos']['y']:.1f})")
        net = x['description'].split('[')[1].split(']')[0] if '[' in x['description'] else '?'
        by_net[net] += 1
    if by_net:
        print('  by net:', dict(by_net.most_common()))
    for v in d['violations']:
        if v['type'] not in COSMETIC:
            print(f"  ! {v['severity']} {v['type']}: " + ' | '.join(
                f"{i['description'][:45]} ({i['pos']['x']:.1f},{i['pos']['y']:.1f})" for i in v['items']))
    shutil.rmtree(tmp, ignore_errors=True) if not a.json else None


if __name__ == '__main__':
    main()
