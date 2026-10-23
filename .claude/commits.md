# Commit log

<!-- Newest first. Format:
## <short-hash> — YYYY-MM-DD — <subject>
What: ...
Why: ...
-->

## pending — 2026-10-23 — PCB v0.2: 50x70 mm two-sided board, buttons, CH569 boot jumper
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
