# Commit log

<!-- Newest first. Format:
## <short-hash> — YYYY-MM-DD — <subject>
What: ...
Why: ...
-->

## pending — 2026-10-23 — JLCPCB BOM and placement files with LCSC part numbers
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
