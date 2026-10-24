"""Remove dangling tracks/vias reported by DRC, repeatedly, without pcbnew's Cleanup tool (system Python).

    /usr/bin/python3 .claude/skills/kicad-board/remove_dangling.py in.kicad_pcb out.kicad_pcb [--keep NET ...]

Why: pcbnew's *Cleanup Tracks & Vias* run with stale zone fills deleted 22 GND fan-out vias (only connected
through the In1 fill) on 2026-10-24. This removes exactly what DRC (zones refilled, project rules) flags as
track_dangling / via_dangling, re-runs DRC, and repeats until nothing dangles, so stub chains go too.
--keep NET: never remove that net's items (a "dangling" segment can be the bridge between a pad and the net,
e.g. VREG_AVDD on v1.0). Check the result with drc_check.py: unconnected must not go up.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile

import pcbnew

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))


def drc(board, tmp):
    rep = os.path.join(tmp, 'drc.json')
    subprocess.run(['kicad-cli', 'pcb', 'drc', '--refill-zones', '--format', 'json', '-o', rep, board],
                   check=True, capture_output=True)
    return json.load(open(rep))


def main():
    args = sys.argv[1:]
    keep = set(args[args.index('--keep') + 1:]) if '--keep' in args else set()
    src, dst = args[0], args[1]
    tmp = tempfile.mkdtemp()
    work = os.path.join(tmp, 'b.kicad_pcb')
    shutil.copy(src, work)
    shutil.copy(os.path.join(ROOT, 'hardware', 'rp569.kicad_pro'), os.path.join(tmp, 'b.kicad_pro'))
    before = len(drc(work, tmp)['unconnected_items'])
    for n in range(10):
        d = drc(work, tmp)
        ids = {i['uuid'] for v in d['violations'] if v['type'] in ('track_dangling', 'via_dangling')
               for i in v['items'] if not any(f'[/{k}]' in i['description'] or f'[{k}]' in i['description']
                                              for k in keep)}
        if not ids:
            break
        b = pcbnew.LoadBoard(work)
        for t in list(b.GetTracks()):
            if t.m_Uuid.AsString() in ids:
                b.Remove(t)
        pcbnew.ZONE_FILLER(b).Fill(b.Zones())
        b.Save(work)
        print(f'pass {n + 1}: removed {len(ids)}')
    after = len(drc(work, tmp)['unconnected_items'])
    print(f'unconnected {before} -> {after}')
    if after > before:
        sys.exit('more unconnected than before: not written (add the bridging net to --keep)')
    shutil.copy(work, dst)
    print('written', dst)


if __name__ == '__main__':
    main()
