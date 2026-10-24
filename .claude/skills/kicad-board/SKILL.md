---
name: kicad-board
description: Use when changing the rp569 hardware (parts, nets, footprints, placement, routing) — the board.py → KiCad generator workflow, its checks, and the Freerouting step.
---

# rp569 KiCad workflow

Everything in `hardware/` is generated. Never edit the `.kicad_sch` / `.kicad_pcb` by hand before hand routing
starts (`CLAUDE.md`).

## Pipeline

| Step | Script | Notes |
|---|---|---|
| Symbols | `gen/mk_lib.py` | CH569W from phantomdrive (pin types fixed), TPS2121 drawn from its datasheet |
| Footprints | `gen/mk_fp.py` | GSB343K33HR, TI RUX0012A: from the datasheet land pattern; lib registered in `hardware/fp-lib-table` |
| Schematic | `gen/gen_sch.py` | one sheet, label per pin, LCSC/MPN fields |
| PCB | `gen/gen_pcb.py` (system `/usr/bin/python3`) | fixed parts dict + two-sided nearest-spot placer; `main(out=, rot={value: deg})` for experiments |
| Net classes | `gen/gen_pro.py` | merges into the KiCad-written `.kicad_pro` (kicad-cli rewrites it, so `build.sh` runs it twice) |
| BOM/CPL | `gen/gen_bom.py` | JLCPCB files in `hardware/jlcpcb/` |
| Checks | `gen/build.sh` | ERC, DRC with schematic parity → `hardware/reports/` |
| Routing (main) | `gen/route_krt.sh <workdir>` | KiCadRoutingTools chain on a copy, DRC against the real project rules |
| Routed board | `hardware/rp569-routed.kicad_pcb` + `.kicad_pro` | v1.0: KRT result finished by hand; the board to fabricate. `rp569.kicad_pcb` stays the generated, unrouted one |
| Header labels | `/usr/bin/python3 gen/add_header_labels.py [pcb]` | Mirrored B.SilkS pin labels beside every PinHeader, plus `BUTTON_LABELS` (SW1 `RPDFU`, SW5 `BOOT`) on F.SilkS in place of the reference (group `header_labels`, re-run replaces them; reports overlaps; uses drawn-text extent, not GetBoundingBox). Re-run after moving a header, before `gen_fab.py` |
| Round corners | `/usr/bin/python3 gen/round_corners.py [pcb]` | Edge.Cuts chamfers → tangent arcs (no-op when already round); gen_pcb.py draws arcs itself |
| Images | `gen/gen_renders.py [pcb] [--tag v1.0]` | 3D renders + layer/assembly plots (SVG, PNG via Inkscape) → `docs/images/`, ~2 min |
| Fab outputs | `gen/gen_fab.py` | DRC gate (0 unconnected, 0 errors) → gerbers + drill → `jlcpcb/rp569-gerbers.zip`, then BOM/CPL (CPL from the routed board) |
| Routing (option) | `/usr/bin/python3 gen/route.py [passes] [pcb] [--keep]` | patched Freerouting 2.1.0 (Java 21), GND fan-out vias, DSN → SES, GND fills on In2/F/B; `--keep` continues from the current routing |

Targets after `build.sh`: ERC 0, DRC 0 electrical, parity 0. Silk overlaps are cosmetic.

**After a `board.py` change** the routed board is stale: `build.sh` regenerates `rp569.kicad_pcb` only. Either
re-route (`route_krt.sh`, then hand finishing) or apply the change in pcbnew on `rp569-routed.kicad_pcb` with
*Update PCB from Schematic*. `build.sh` also re-runs `gen_bom.py`, which takes the CPL from the routed board.

## Helper scripts (this folder)

| Script | Python | Use |
|---|---|---|
| `drc_check.py [board] [--pro P] [--json out]` | any | DRC against the **real** project rules (copies the board next to `hardware/rp569.kicad_pro`), lists unconnected links with coordinates and non-cosmetic violations. Default board: `hardware/rp569-routed.kicad_pcb` |
| `board_metrics.py [board]` | `/usr/bin/python3` | HSPI lengths vs RCLK, USB pair skew/vias, In1 GND fill islands, tracks on In1, USB length not over In1 and why |
| `remove_dangling.py in out [--keep NET..]` | `/usr/bin/python3` | Safe replacement for pcbnew's Cleanup: removes DRC-dangling items by uuid, repeats, refuses to write if unconnected goes up |

## Hand-editing loop with the user (v1.0 was finished this way)

The user edits `hardware/rp569-routed.kicad_pcb` in pcbnew and asks "check my edits": run `drc_check.py`, report
progress (N → M unconnected), what is done, and the next links with coordinates and a layer suggestion. Watch for:
tracks put on In1 by mistake (In1/In2 are adjacent in the layer list; the user accepted In1 tracks for v1.0,
D-021, so check `board_metrics.py` keeps In1 one island and USB gaps are only via antipads), and new unconnected
items after a Cleanup (stale zone fills). Never write the board while KiCad has it open (`~*.lck` next to it).
After edits: `python3 hardware/gen/gen_fab.py` to refresh the JLCPCB files.

## Gotchas found so far

- Board net names carry the schematic's `/` prefix (`/GND`); strip it when matching against `board.py` nets.
- Every new symbol/footprint field must exist on both sides or parity reports `footprint_symbol_field_mismatch`.
- ERC: a pin tied to GND clashes with the USB3 symbol's GND `power_out` pin if it is `open_collector`/`output`.
- Footprint orientation: check pad positions (`fp.Pads()`) to know which side is the connector opening before
  choosing a rotation (USB-C HRO opens toward +y at 0°; KiCad rotation is CCW on screen).
- `SetLayerAndFlip(B_Cu)` around the origin, then position; bounding boxes change after the flip.
- Freerouting: In1 typed `LT_POWER` (GND plane, reached by vias); zones on signal layers would be exported
  as obstacles, so `route.py` removes them before export and adds the GND fills after import.
- Freerouting does not do coupled diff pairs: route USB3/USB2 pairs in pcbnew (diff-pair router), then
  autoroute the rest.
- Experiments: generate variants into the scratchpad with absolute paths and run several `route.py` in parallel
  (8 cores, ~45 s per pass each). Saving a board rewrites the `.kicad_pro` next to it with default rules
  (0.2/0.2 mm, no classes); `route.py` therefore copies `hardware/rp569.kicad_pro` next to any variant board and
  aborts if the DSN has no `(class Power`. Always check `(width`/`(clearance` in the DSN before trusting a result.
- Stock Freerouting 2.1.0 ignores `-mp` (runs to its 12 h job timeout) and a job timeout stops it mid-pass with
  connections ripped up. `route.py` uses `~/.local/share/freerouting/freerouting-2.1.0-mp.jar`, built by
  `gen/patch_freerouting_mp.py` (see its docstring); rebuild it if the jar is missing.
- Freerouting places no via for the In1 plane net: `route.py:gnd_fanout()` adds the GND vias before export.
- Never run `build.sh` while `route.py` is running: route.py saves over `rp569.kicad_pcb` at the end, with the
  board it loaded at the start.

- pcbnew *Cleanup Tracks & Vias*: refill zones (**B**) first, or it deletes the GND fan-out vias (connected only
  through the In1 fill) as "connected on one layer". Check the preview count, DRC afterwards.

## Rule: watch every routing run (user request, 2026-10-24)

Whenever Freerouting runs (route.py or a direct `java -jar`), start a **Monitor** that reports progress
**every 3 minutes** until it stops, and relay each report to the user in one line (time, pass, unrouted):

```bash
L=/home/kelu/projets/rp569/hardware/route.log      # or the variant's log
sleep 40
while true; do
  last=$(grep -a -o "pass #[0-9]* .*([0-9]* unrouted" "$L" 2>/dev/null | awk '{print $2, $(NF-1)")"}' | tail -1)
  if pgrep -f "[j]ava -jar .*freerouting" >/dev/null; then echo "$(date +%H:%M) running: ${last:-no pass yet}";
  else sleep 60; echo "$(date +%H:%M) Freerouting stopped. last ${last:-none}. log tail: $(grep -av 'property.h' "$L" | tail -1 | cut -c1-160)"; exit 0; fi
  sleep 180
done
```

- Monitor `timeout_ms` 1800000 (the maximum); re-arm it when it expires while the router is still running.
- Stop the previous watch (TaskStop) before starting a new run that writes the same log.
- `pgrep -f "[j]ava ..."`: the bracket keeps the pattern from matching the shell running it (same for `pkill`).
- If the count stops falling for ~10 passes, analyse the leftovers (`kicad-cli pcb drc --format json` →
  `unconnected_items`, grouped by net and footprint) instead of waiting longer.
