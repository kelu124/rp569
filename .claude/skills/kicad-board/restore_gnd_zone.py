#!/usr/bin/python3
"""Put the In1 GND plane back on the routed board (needs the system python with pcbnew).

    /usr/bin/python3 .claude/skills/kicad-board/restore_gnd_zone.py [board.kicad_pcb]

The v1.0 zone: net GND on In1.Cu, rectangle inset 0.5 mm from the 50x70 mm outline, clearance 0.1 mm,
min width 0.1 mm, thermal reliefs (gap 0.2, spoke 0.2). Refuses to run if the board already has an In1 zone
or if KiCad holds the file (lock file). Refills and saves in place. Reload the board in pcbnew afterwards
(File > Revert): a pcbnew session that was opened before this script will drop the zone again on its next save.
"""
import sys
from pathlib import Path

import pcbnew

ROOT = Path(__file__).resolve().parents[3]
board_path = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "hardware" / "rp569-routed.kicad_pcb")
lock = board_path.with_name("~" + board_path.name + ".lck")
if lock.exists():
    sys.exit(f"{lock} exists: close the board in KiCad first")
b = pcbnew.LoadBoard(str(board_path))
if any(z.IsOnLayer(pcbnew.In1_Cu) and not z.GetIsRuleArea() for z in b.Zones()):
    sys.exit("the board already has an In1 zone; nothing to do (press B in pcbnew to refill)")

mm = pcbnew.FromMM
z = pcbnew.ZONE(b)
z.SetLayer(pcbnew.In1_Cu)
z.SetNet(b.FindNet("/GND") or b.FindNet("GND"))
z.SetLocalClearance(mm(0.1))
z.SetMinThickness(mm(0.1))
z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
z.SetThermalReliefGap(mm(0.2))
z.SetThermalReliefSpokeWidth(mm(0.2))
z.SetAssignedPriority(0)
ol = z.Outline()
ol.NewOutline()
for x, y in [(0.5, 0.5), (49.5, 0.5), (49.5, 69.5), (0.5, 69.5)]:
    ol.Append(mm(x), mm(y))
b.Add(z)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(str(board_path), b)
fill = z.GetFilledPolysList(pcbnew.In1_Cu)
print(f"In1 GND zone restored and filled in {board_path}: {fill.OutlineCount()} piece(s)")
