# DONE

<!-- Newest first. Format: - YYYY-MM-DD — what was done (commit hash if any) -->

- 2026-11-19 — USB3 investigated (D-032): USB3 cable + 5 Gb/s port confirmed; CH569 link counters (console `usb3`) show termination detect + LFPS + TXEQ on every attempt, LINK_RDY never; 4 PHY settings tried, same; pins, AC caps, crystal load checked; routed SS pairs measured: SSRX skew 6.5 mm with 3 vias on N only, SSTX 5.3 mm -> board v1.1 routing fix.
- 2026-11-19 — Firmware variant 1 "stream" (D-031): RP2350 PIO HSPI sender + CH569 HSPI receiver -> USB2 bulk -> host checker; 16.00 MB/s sent, 16.05 MB/s received, every word checked, 0 gaps in steady state (20 s runs). Found: HRVLD timing, header layout, CRC-16/USB. Hook bumps both variant version files.
- 2026-11-19 — RST# and SD_DET tested (D-030): RESET_EN set on board #1 (patched wch-ch56x-isp `-e=on`), `ch reset` now resets the CH569 (re-enumerates, flags announced), releases the card first; card detect low = in, hot swap in drive mode (removal -> release, insertion -> new card mounted; red LED follows), `CH569 card: in|out` on the RP2350 port, `ch status` card field, console `sd` det field.
- 2026-11-19 — SD benchmark log: console `sd` (CID, size, clock) + python `sd_info()`/`decode_cid()`, `firmware/tools/sd_bench.py` -> `firmware/ch569/sd_benchmarks.csv` (date, firmware x.y.z, card CID fields, MB/s, bad sectors). New card 0xC9 "MSSD0" 59.7 GB: 7.1 MB/s write, 17.1 MB/s read; ACMD23 pre-erase slower (6.7), off. Versioning roles recorded (patch = Claude/hook, minor = user, major = firmware variant shared by both chips).
- 2026-11-19 — Drive mode at full speed (D-029, `msc_fast.c`, phantomdrive port): 5.0 MB/s write, 17.6 MB/s read on USB2, 32 MB of numbered sectors verified (an OUT-flag race corrupting 28/512 sectors found and fixed). Mode flags: USB product "rp569 CH569 drive|serial", link `\x01MDRV`/`\x01MSER` -> RP2350 "CH569 mode: ..." + `ch status` mode; python `ports.ch569_usb_mode()`, `ch569_mode_events()`, `ch_status()['mode']`.
- 2026-11-19 — CH569 drive / serial modes (D-028): drive = MSC only, serial = link + console CDC; switch on host eject / `ch eject` and console `mount` / `ch mount` (`\x01SDON`), in place (no reset). RP2350 `ch mount`, python `ch_mount`, console `mount`, `python -m rp569 mount`, ports on if0/if2. Tested on board #1: eject, console mount, ch eject, ch mount, ch cycle.
- 2026-11-19 — CH569 drive speed: 0.25 → 3.8 MB/s write, 1.06 → 8.4 MB/s read (USB2, 32 MB verified): multi-block SD read/write in 64-sector chunks, CMD6 high-speed 48 MHz (24 MHz fallback), CSW queued after a bulk-OUT stall, 1024-byte SuperSpeed MSC packets, `sd clock` in console `status` (+ python).
- 2026-11-19 — Clean SD release before every CH569 power-off: link control lines (`\x01SDREL` / `\x01SDOFF`), RP2350 `ch eject` + release in `ch off/cycle/boot`, CH569 waits for the MSC engine and the card (DAT0); host `python -m rp569 eject` (udisksctl unmount, then release). Tested on board #1: eject → host sees 0 B and drops the mount, cycle → card back.
- 2026-11-19 — CH569 USB2 fallback = same composite as USB3 (drive + link CDC + console CDC, product "rp569 CH569", UID serial): rewritten `CH56x_usb20.c`, speed-aware CDC pumps, descriptor checker covers both. Fixed: MSC WRITE10 left EP3 NAKed (30 s timeout + reset per write), console prompt stuck in stdio buffer, host line coding moving UART1 to 9600. Verified on board #1: FAT32 card mounts, 32 MB write/read identical, link echo 20/20, bridge both ways.
- 2026-11-19 — First firmware on board #1: RP2350 flashed and working (CLI, LEDs, buttons, ADC, `ch boot`/`ch cycle`); CH569 config DEBUG_EN off + download pin PA13 → PA5, flashed with wch-ch56x-isp (wchisp 0.3.0 cannot flash bootloader 2.70), unattended reflash checked; UART link echo 20/20, `ch pins` probe, 0x00 power-up glitch filtered; `check_mem.py` RAMX/USER_MEM guard in the CH569 build; udev rules; `docs/toolchain.md`. Open: CH569 enumerates at USB2 only.
- 2026-11-14 — Licenses added, same as rp417/pic0rick: `LICENSE.txt` (TAPR OHL, hardware), `LICENSE-GPL-3.0.txt` (software), `LICENSE-CC-BY-SA-3.0.txt` (docs); License section in README and docs/index.md.
- 2026-11-02 — Last 3 vias off passive pads (R9, C28, R48), silkscreen text; via-in-pad list empty for passives; JLCPCB files + renders regenerated.
- 2026-11-02 — Vias off passive pads (42 → 1 inside + 2 on the edge: R9, C28, R48), last 0.1 mm microvia removed, In1 GND
  zone restored (lost in 9b067cd) and plane pieces stitched; 0 unconnected / 0 DRC errors; JLCPCB files regenerated;
  skill scripts `via_check.py`, `restore_gnd_zone.py`.

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
