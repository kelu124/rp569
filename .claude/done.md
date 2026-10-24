# DONE

<!-- Newest first. Format: - YYYY-MM-DD — what was done (commit hash if any) -->

- 2026-10-24 — Python host library `python/rp569` (port roles by VID/PID/product/UID/interface, one method per command), `version` + chip UID on both firmwares, version.yaml, pre-commit hook (command sync + patch bump), CH569 console, Hackaday drafts 03/04 (D-025).
- 2026-10-24 — First firmware (D-024): `firmware/rp2350` USB CLI (LED, `dfu`, CH569 power/boot/reset/link) and `firmware/ch569` (CH569_Composite fork: USB3 serial bridged to the RP2350 link with echo, SD drive); both build; skill `firmware`.
- 2026-10-24 — README/docs image links moved to the regenerated `docs/images/` renders; stale `docs/img/pcb-render-v1.0-*` removed.
- 2026-10-24 — Rounded board corners (`gen/round_corners.py`, gen_pcb arcs), SW1 `RPDFU` / SW5 `BOOT` silkscreen labels, gerbers and renders regenerated (D-023).
- 2026-10-24 — Header pin labels on the bottom silkscreen (`gen/add_header_labels.py`), gerbers and renders regenerated.
- 2026-10-24 — GitHub Pages site in `docs/` (index, architecture, production pages; power.md updated to v1.0).
- 2026-10-24 — Renders in `docs/images/` (`gen/gen_renders.py`: 3D views + layer plots).
- 2026-10-24 — Handoff for a later session (`.claude/handoff.md`), check scripts saved in the kicad-board skill, two Hackaday log drafts in `docs/hadposts/`.
- 2026-10-24 — JLCPCB production files for v1.0 (`gen/gen_fab.py` → `hardware/jlcpcb/rp569-gerbers.zip`, BOM, CPL), routed board renamed `hardware/rp569-routed.kicad_pcb`, v1.0 renders, docs updated (D-022).
- 2026-10-24 — v1.0: board hand-finished by the user (KRT result + manual links, In1 tracks accepted, D-021): 0 unconnected, 0 DRC errors.
- 2026-10-24 — D-020: TPS2121 moved next to the USB-C, sense dividers at their rail, pull-ups at their chip; KRT 16 of 405 links left, 0 electrical violations.
- 2026-10-24 — Second KRT route pass: 21 of 405 links left, 0 electrical violations → `hardware/autoroute/rp569-krt.kicad_pcb`.
- 2026-10-24 — D-019: HSPI and Power clearance 0.1 mm; `gen/route_krt.sh` (KiCadRoutingTools chain): 27 of 405 links left, 0 clearance violations against the project rules.
- 2026-10-24 — KiCadRoutingTools evaluated (`~/tools/KiCadRoutingTools`): fab-floor relaxation, In1 use and class-clearance lowering found and disabled.
- 2026-10-24 — D-018: HSPI series resistors dropped, RP2350 GPIO24–39 and CH569 generic GPIOs reassigned by chip side, buttons/LEDs moved to the edges (`docs/pinmap.md` v0.2).
- 2026-10-24 — Freerouting set up (`gen/route.py`): net classes guard, GND fan-out vias, Rail class, patched 2.1.0 jar so `-mp` works; best 35 links left. Kept as a documented option.
- 2026-10-24 — D-017: v0.3 placement from pin sides (RP2350 90°, CH569 270°), passives turned toward their nets.
- 2026-10-23 — D-015: TPS2121 power mux replaces the 2 × LM66100 (symbol + RUX0012A footprint from the datasheet), LCSC C485916.
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
