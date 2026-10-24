# Commit log

<!-- Newest first. Format:
## <short-hash> — YYYY-MM-DD — <subject>
What: ...
Why: ...
-->

## pending — 2026-10-24 — Session handoff: helper scripts, handoff note, Hackaday drafts
What: .claude/handoff.md (state, next steps, tools, agreements; linked from CLAUDE.md), kicad-board skill helper
scripts (drc_check.py, board_metrics.py, remove_dangling.py) + hand-editing loop, docs/hadposts/ (2 Hackaday log
drafts + index), README layout row, logs.
Why: user saves the session to resume later and asked for scripts/learnings kept, plus Hackaday drafts.

## ea0e4c9 — 2026-10-24 — v1.0 production files: routed board next to the schematic, JLCPCB gerbers/BOM/CPL
What: hardware/autoroute/rp569-krt.* → hardware/rp569-routed.kicad_pcb/.kicad_pro; gen/gen_fab.py (DRC gate,
gerbers, drill, zip, BOM/CPL); gen_bom.py takes the CPL from the routed board; hardware/jlcpcb/ (rp569-gerbers.zip,
gerbers/, BOM, CPL), reports/drc-routed.json; v1.0 renders; specification D-022 + R-18; README, CLAUDE.md,
docs/layout.md, docs/bom.md; skills kicad-board and jlcpcb-parts.
Why: user asked for the routed board next to the schematic, JLCPCB outputs, and the docs updated.

## ab8df3d — 2026-10-24 — v1.0: hand-finished board, fully connected
What: hardware/autoroute/rp569-krt.kicad_pcb (hand-routed last links incl. tracks on In1, D-021; dangling copper
removed; "rp569 v1.0" silkscreen), specification D-021, docs/layout.md status, .gitignore (*.lck), logs. Tag v1.0.
Why: user asked to commit as v1. DRC: 0 unconnected, 0 errors (warnings: silkscreen, 1 VREG_AVDD stub).
A KiCad "Cleanup Tracks & Vias" run with stale zone fills had deleted 22 GND fan-out vias; the board was rebuilt
from the 16:26 save minus the dangling items plus the new silkscreen text.

## 2d1be9a — 2026-10-24 — D-020 placement: corridor between the chips cleared; autorouted board at 16 links
What: gen_pcb.py (TPS2121 fixed at the USB-C, PMUX parts follow it, sense dividers at their rail, pull-ups at
their chip), route_krt.sh (second route pass built in), regenerated hardware + reports + BOM/CPL,
hardware/autoroute/rp569-krt.kicad_pcb, specification D-020, docs/layout.md, logs.
Why: user chose path 1 (fix the placement before hand finishing); 21 → 16 links left.

## 555a41c — 2026-10-24 — Autorouted board (KiCadRoutingTools, 21 links left)
What: hardware/autoroute/rp569-krt.kicad_pcb (+ project copy, fp-lib-table pointing at ../lib), docs/layout.md
trial table and status, logs.
Why: user asked to continue the KRT routing and commit; starting point for hand finishing (todo: routing).

## 250594e — 2026-10-24 — TPS2121 power mux, v0.3 placement, pin reassignment, autorouting setups
What: board.py (TPS2121 replaces the LM66100s; HSPI series arrays dropped; RP2350/CH569 pins reassigned by side),
mk_lib/mk_fp (TPS2121 symbol + RUX0012A footprint), gen_pcb (v0.3 layout, buttons/LEDs at the edges, passive
orientation, hole clearance 0.2), gen_pro (Rail class, HSPI/Power 0.1 mm clearance), gen/route.py (Freerouting with
GND fan-out vias, patched jar), gen/patch_freerouting_mp.py, gen/route_krt.sh (KiCadRoutingTools chain), regenerated
hardware files and reports, specification (D-015..D-019, R-06/08/10/11/15), docs (pinmap, layout, power, bom),
project skills kicad-board and jlcpcb-parts, datasheet/TPS2121.
Why: user asked for the TPS2121, routing and JLCPCB; routability fixes D-017..D-019. Board still unrouted in git
(routing is done on copies: 27 links left with KRT, 35 with Freerouting).

## 9b192cc — 2026-10-23 — JLCPCB BOM and placement files with LCSC part numbers
What: board.py LCSC table + LED/resistor/inductor changes, gen_sch/gen_pcb LCSC+MPN fields, gen/gen_bom.py,
hardware/jlcpcb/ (BOM, CPL), docs/bom.md, specification (R-18, D-014, D-005), todo/done.
Why: User plans to have the board made and assembled by JLCPCB.

## ad16ba7 — 2026-10-23 — USB3 Micro-B with through-hole shell legs (Amphenol GSB343K33HR)
What: gen/mk_fp.py + lib/rp569.pretty footprint, fp-lib-table, gen_pcb loads project footprints, board.py,
build.sh, specification (R-16 met, D-013), datasheet/GSB343K33HR (added by the user), renders.
Why: User wants a firm mechanical connection for the USB3 connector.

## a8a11e0 — 2026-10-23 — Add specification.md (requirements + decisions)
What: specification.md (R-01..R-17, D-001..D-013), removed .claude/decisions.md, updated CLAUDE.md, project-logs
skill, README, todo/done. D-004 rewritten with the checked RP2350 → CH569 bootloader sequence; D-013 opened
for a through-hole-leg USB3 Micro-B.
Why: User asked for one specification file with decisions and requirements, a sturdier USB3 connector, and
confirmation that the RP2350 can trigger the CH569 bootloader like the BOOT button.

## 4859eb2 — 2026-10-23 — PCB v0.2: 50x70 mm two-sided board, buttons, CH569 boot jumper
What: board.py (RP BTN1/2 on GPIO30/31, CH BTN1/2 on PB3/PB4, CH BOOT JP, RP UART → RP EXT, CH EXT 2x07);
gen_pcb.py (50×70 mm, 3 mm chamfers, fixed top-side layout, two-sided nearest-spot placer, net-prefix fix
in the placement targets); docs (layout, pinmap, README), D-011/D-012, renders v0.2.
Why: User asked for a smaller board with angled corners, parts on both sides, a boot jumper, user buttons
and LEDs next to their chips.

## 94ec323 — 2026-10-23 — KiCad v0.1: generated schematic + placed PCB
What: hardware/gen/ (board.py netlist, mk_lib, gen_sch, gen_pcb, gen_pro, build.sh), hardware/ KiCad project + lib + schematic PDF, reports, docs/layout.md, render image, decisions D-008..D-010.
Why: User asked to try schematics and layout in KiCad.

## e4f914b — 2026-10-23 — Power architecture (D-007)
What: docs/power.md, pinmap (power control/sense GPIOs), decisions D-006/D-007, README, CLAUDE.md, todo/done.
Why: User asked for an RP2350-controlled CH569 supply from its USB3 VBUS and for ideal diodes against back-feed.

## 4333bfd — 2026-10-23 — Pin map v0.1 and CH569 hardware notes
What: docs/pinmap.md, docs/ch569-hardware-notes.md, decisions D-004..D-006, todo/done.
Why: Verified pinout from the PDFs; captured phantomdrive's proven CH569 power/USB3/SD design.

## 993a985 — 2026-10-23 — Decide D-003: 8-bit HSPI + 4-bit SD
What: decisions D-002/D-003, CLAUDE.md, README, todo/done.
Why: User chose option (a) to resolve the HSPI/SD pin conflict.

## 1af2c7d — 2026-10-23 — Datasheets, phantomdrive reference, logic analyser requirement, D-003
What: datasheet/ (PDF+md), project/phantomdrive snapshot, README/CLAUDE.md requirements (logic analyser), decisions D-003, todo updates.
Why: Gather source docs; record the HSPI/SD pin conflict found in CH569DS1.

## 1bb4d65 — 2026-10-23 — Project scaffolding, README, logs, HydraUSB3 reference snapshots
What: README.md (project overview), CLAUDE.md, .claude/ (todo, done, commits, decisions, project-logs skill), project/ with HydraUSB3 hw/fw/lib/bsp/isp snapshots.
Why: Set up project context, logging workflow and CH569 reference material.

## 7a66ad8 — 2026-10-23 — Initial commit
What: README.md only.
Why: Repo creation.
