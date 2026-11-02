#!/usr/bin/python3
"""Via audit of a routed board (needs the system python with pcbnew).

    /usr/bin/python3 .claude/skills/kicad-board/via_check.py [board.kicad_pcb]

Reports: via (drill, diameter) sizes, every via whose hole sits inside / touches a pad
(solder wicking at reflow; the RP2350B/CH569 exposed-pad thermal vias are expected),
and the In1 GND fill islands (how many pieces the plane is in, and where).
Default board: hardware/rp569-routed.kicad_pcb.
"""
import math
import sys
from collections import Counter
from pathlib import Path

import pcbnew

ROOT = Path(__file__).resolve().parents[3]
board_path = sys.argv[1] if len(sys.argv) > 1 else str(ROOT / "hardware" / "rp569-routed.kicad_pcb")
b = pcbnew.LoadBoard(board_path)
vias = [t for t in b.GetTracks() if t.GetClass() == "PCB_VIA"]

print(f"{board_path}: {len(vias)} vias")
sizes = Counter((v.GetDrillValue() / 1e6, v.GetWidth(pcbnew.F_Cu) / 1e6) for v in vias)
for (d, w), n in sorted(sizes.items()):
    print(f"  drill {d:.2f} / dia {w:.2f} mm: {n}" + ("" if (d, w) == (0.3, 0.5) else "   <-- not the JLCPCB standard 0.3/0.5"))
for v in vias:
    if v.GetViaType() != pcbnew.VIATYPE_THROUGH:
        p = v.GetPosition()
        print(f"  non-through via at ({p.x/1e6:.2f},{p.y/1e6:.2f}) net {v.GetNetname()}")

rows = []
for v in vias:
    pos = v.GetPosition()
    r = v.GetWidth(pcbnew.F_Cu) // 2
    dr = v.GetDrillValue() // 2
    for fp in b.GetFootprints():
        for p in fp.Pads():
            bb = p.GetBoundingBox()
            if not bb.Contains(pos) and bb.Distance(pos) > r:
                continue
            lyr = pcbnew.F_Cu if p.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu
            shape = p.GetEffectiveShape(lyr)
            if not shape.Collide(pos, r):
                continue
            if p.HitTest(pos):
                cls = "HOLE-INSIDE"
            elif shape.Collide(pos, dr):
                cls = "HOLE-TOUCH"
            else:
                continue  # only the annular ring overlaps the pad: fine
            ref = fp.GetReference()
            kind = "IC" if ref[0] == "U" else ("passive" if ref[0] in "RCL" else "other")
            rows.append((cls, kind, ref, p.GetNumber(), p.GetNetname(), "F" if lyr == pcbnew.F_Cu else "B",
                         pos.x / 1e6, pos.y / 1e6))
rows.sort()
print("\nVias in pads (hole inside / touching the pad copper):")
for cls, kind, ref, num, net, side, x, y in rows:
    print(f"  {cls:12} {kind:8} {ref:5} pad {num:3} {net:14} {side} @({x:.2f},{y:.2f})")
print("  summary:", dict(Counter((cls, kind) for cls, kind, *_ in rows)))

zones = [z for z in b.Zones() if z.IsOnLayer(pcbnew.In1_Cu) and not z.GetIsRuleArea()]
print("\nIn1 GND zone:", "MISSING (add it back, see restore_gnd_zone.py)" if not zones else "")
for z in zones:
    fill = z.GetFilledPolysList(pcbnew.In1_Cu)
    print(f"  net {z.GetNetname()} filled={z.IsFilled()} pieces={fill.OutlineCount()}")
    for i in range(fill.OutlineCount()):
        o = fill.Outline(i)
        bb = o.BBox()
        gv = [t for t in vias if t.GetNetname() == z.GetNetname() and o.PointInside(t.GetPosition())]
        print(f"    piece {i}: {o.Area()/1e12:7.1f} mm2 at ({bb.GetLeft()/1e6:.1f},{bb.GetTop()/1e6:.1f})-"
              f"({bb.GetRight()/1e6:.1f},{bb.GetBottom()/1e6:.1f}), {len(gv)} GND vias")
