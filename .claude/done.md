# DONE

<!-- Newest first. Format: - YYYY-MM-DD — what was done (commit hash if any) -->

- 2026-10-23 — JLCPCB BOM + CPL with LCSC numbers (`gen/gen_bom.py`, `docs/bom.md`, D-014): RGB LED → LTST-C19HE1WT (pinout checked, G/B 100 Ω), 2.2 µH → FTC303020D2R2MBCA; 3 consigned parts.
- 2026-10-23 — USB3 Micro-B swapped to Amphenol GSB343K33HR (THT shell legs, R-16/D-013): footprint generated from the drawing by `gen/mk_fp.py`, `hardware/fp-lib-table`.
- 2026-10-23 — `specification.md`: requirements R-01..R-17 + all design decisions (replaces `.claude/decisions.md`). Checked on the netlist that the RP2350 can force the CH569 bootloader like the BOOT button (D-004 rewritten).
- 2026-10-23 — PCB v0.2: 50×70 mm, chamfered corners, two-sided placement; CH569 boot jumper; 2 user buttons per chip; LEDs next to their chip; RP UART merged into RP EXT, CH EXT 2x07 (D-011, D-012). DRC: 0 electrical errors, parity 0.
- 2026-10-23 — KiCad v0.1: generator (`hardware/gen/`), schematic (144 symbols, ERC: 1 expected error), PCB placed 4-layer 100×70 mm (DRC: 0 electrical errors, parity 0), net classes, `docs/layout.md`. D-008/9/10.
- 2026-10-23 — `docs/power.md`: ideal-diode OR + RP2350-controlled CH569 load switch (D-007); pin map updated (GPIO39/46/47).
- 2026-10-23 — Verified D-003 pinout in CH569DS1.pdf; wrote `docs/pinmap.md` v0.1 and `docs/ch569-hardware-notes.md` (phantomdrive netlist study); decisions D-004/5/6.
- 2026-10-23 — D-003 decided: 8-bit HSPI + 4-bit SD card (option a).
- 2026-10-23 — `datasheet/`: RP2350 datasheet + HW design guide, CH569DS1, QingKe V3 manual, SD spec (PDF + markitdown .md).
- 2026-10-23 — Added phantomdrive snapshot to `project/` (CH569 + USB3 + SD KiCad reference).
- 2026-10-23 — Found HSPI 16-bit / eMMC-SD pin conflict on CH569 (D-003).
- 2026-10-23 — Rewrote README.md with project overview, planned features and repo layout.
- 2026-10-23 — Added HydraUSB3 reference snapshots (hw, fw, lib, bsp, isp) under `project/hydrausb3/`.
- 2026-10-23 — Created `CLAUDE.md` and `.claude/` scaffolding (todo, done, commits, decisions, project-logs skill), and `project/` folder for reference projects.
