"""Signal-integrity metrics of the routed board (needs the system Python: /usr/bin/python3, imports pcbnew).

    /usr/bin/python3 .claude/skills/kicad-board/board_metrics.py [board.kicad_pcb]

- HSPI lengths vs HSPI_RCLK (HSPI_D0 also feeds the CH569 boot button/jumper, so it reads long).
- USB pair P/N lengths, skew and vias (targets: USB2 skew < ~1 mm, USB3 SS a fraction of a mm, no vias).
- In1 GND plane: number of fill islands, tracks on In1 (D-021), and the share of each USB pair's F.Cu length that
  is not over In1 copper, with the cause (In1 track slot or via antipad).
"""
import collections
import math
import os
import sys

import pcbnew

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
PAIRS = [('USB3_SSRX_P', 'USB3_SSRX_N'), ('USB3_SSTX_P', 'USB3_SSTX_N'), ('CH_SSTX_P', 'CH_SSTX_N'),
         ('USB3_DP', 'USB3_DM'), ('USBC_DP', 'USBC_DM'), ('RP_USB_DP', 'RP_USB_DM')]


def seg_dist(p, t):
    s, e = t.GetStart(), t.GetEnd()
    dx, dy = e.x - s.x, e.y - s.y
    ll = dx * dx + dy * dy
    u = 0 if ll == 0 else max(0, min(1, ((p.x - s.x) * dx + (p.y - s.y) * dy) / ll))
    return math.hypot(p.x - s.x - u * dx, p.y - s.y - u * dy) / 1e6


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'hardware', 'rp569-routed.kicad_pcb')
    b = pcbnew.LoadBoard(path)
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    length, vias = collections.Counter(), collections.Counter()
    for t in b.GetTracks():
        n = t.GetNetname().lstrip('/')
        if t.GetClass() == 'PCB_TRACK':
            length[n] += t.GetLength() / 1e6
        elif t.GetClass() == 'PCB_VIA':
            vias[n] += 1
    ref = length['HSPI_RCLK']
    print(f'HSPI lengths vs RCLK {ref:.1f} mm:')
    for n in sorted((n for n in length if n.startswith('HSPI_')), key=lambda n: length[n]):
        print(f'  {n:11} {length[n]:5.1f}  {length[n] - ref:+5.1f}  vias {vias[n]}')
    print('USB pairs (P / N mm, skew, vias):')
    for p, q in PAIRS:
        print(f'  {p[:-2]:10} {length[p]:5.1f} / {length[q]:5.1f}  skew {abs(length[p] - length[q]):4.1f}  '
              f'vias {vias[p]}/{vias[q]}')
    zone = next(z for z in b.Zones() if z.IsOnLayer(pcbnew.In1_Cu) and not z.GetIsRuleArea())
    fill = zone.GetFilledPolysList(pcbnew.In1_Cu)
    in1 = [t for t in b.GetTracks() if t.GetClass() == 'PCB_TRACK' and t.GetLayer() == pcbnew.In1_Cu]
    allvias = [t for t in b.GetTracks() if t.GetClass() == 'PCB_VIA']
    print(f'In1 GND fill: {fill.OutlineCount()} island(s); {len(in1)} track segments on In1')
    for p, q in PAIRS:
        for n in (p, q):
            tot = gap = 0
            cause = collections.Counter()
            for t in b.GetTracks():
                if t.GetClass() != 'PCB_TRACK' or t.GetLayer() != pcbnew.F_Cu or t.GetNetname().lstrip('/') != n:
                    continue
                s, e = t.GetStart(), t.GetEnd()
                k = max(1, int(math.hypot(e.x - s.x, e.y - s.y) / pcbnew.FromMM(0.1)))
                for i in range(k + 1):
                    pt = pcbnew.VECTOR2I(int(s.x + (e.x - s.x) * i / k), int(s.y + (e.y - s.y) * i / k))
                    tot += 1
                    if fill.Contains(pt):
                        continue
                    gap += 1
                    if in1 and min(seg_dist(pt, x) for x in in1) < 0.6:
                        cause['In1 track'] += 1
                    elif allvias and min(math.hypot(pt.x - v.GetPosition().x, pt.y - v.GetPosition().y) / 1e6
                                         for v in allvias) < 0.7:
                        cause['via antipad'] += 1
                    else:
                        cause['other'] += 1
            if tot and gap:
                print(f'  {n:12} {100 * gap / tot:4.1f}% not over In1 {dict(cause)}')


if __name__ == '__main__':
    main()
