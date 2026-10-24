# rp569 — specification

Requirements (what the board must do) and design decisions (how, and why). Open questions and tasks are in
`.claude/todo.md`. Pin-level detail: `docs/pinmap.md`, power: `docs/power.md`, layout: `docs/layout.md`.

Status keys: **met** (in the schematic/PCB), **partial**, **open**.

## 1. Requirements

All requested by the user, 2026-10-23.

| ID | Requirement | Status | How / decision |
|---|---|---|---|
| R-01 | KiCad design pairing an RP2350B (QFN-80, 48 GPIO) and a CH569W | met | `hardware/`, generated from `hardware/gen/board.py` (D-010) |
| R-02 | CH569 on a USB 3.0 connector | met | USB3 Micro-B, AC caps on SSTX, ESD on all pairs (D-008) |
| R-03 | SD card on the CH569's SD/eMMC pins | met | 4-bit SD on the eMMC controller (D-003) |
| R-04 | RP2350B on USB 2.0 through a USB-C connector | met | USB-C, 5.1k CC pull-downs, 27 Ω series, ESD |
| R-05 | The RP2350 can program the CH569, or at least enable its programming, for automated testing | met | Power switch + boot pin control (D-004) |
| R-06 | Shared I2C bus between the two chips | met | RP I2C1 GPIO26/27 ↔ CH569 PA14/PA15 (bit-bang), 4.7k pull-ups (D-018) |
| R-07 | Shared UART between the two chips | met | RP UART1 GPIO24/25 ↔ CH569 UART1 PA7/PA8 |
| R-08 | Parallel bus with clock: the RP2350 sends high-speed data, the CH569 exports it over USB 3 | met (8-bit) | CH569 HSPI 8-bit + handshake, direct connection (D-002, D-003, D-018; 16-bit asked first, conflicts with the SD card) |
| R-09 | Logic analyser: 8 RP2350 pins sampled and streamed over the parallel bus | met | GPIO0–7, 100 Ω series, LA IN header |
| R-10 | One RGB LED per chip (first asked close to its chip; 2026-10-24: may sit further away) | met | Discrete RGB on PWM pins (D-005), at the board edge on its chip's half (D-018) |
| R-11 | Breakout I2C bus on each chip, separate from the shared bus (CH569: bit-bang allowed) | met | RP I2C0 GPIO36/37; CH569 PB12/PB13 bit-banged; headers with pull-ups (D-018) |
| R-12 | CH569 powered from its own USB3 VBUS through an RP2350-controlled switch; ideal diodes so the two USB connectors cannot back-feed each other | met | D-007, D-009 |
| R-13 | Board 50×70 mm, angled corners, parts on both sides, main chips on the same side as the USB connectors and the SD socket | met | D-011 |
| R-14 | 2.54 mm 2x1 header for a physical jumper to program a blank CH569 | met | CH BOOT JP, PA5 → 1k → GND (D-004) |
| R-15 | Two user buttons for the RP2350 and two for the CH569 (2026-10-24: may sit away from the chips) | met | RP GPIO31/39, CH569 PA12/PA13, 10k pull-ups, at the board edges (D-018) |
| R-16 | USB3 Micro-B with through-hole legs, for a firm mechanical hold | met | Amphenol GSB343K33HR, project footprint (D-013) |
| R-17 | The RP2350 can put the CH569 in its bootloader the same way as the BOOT button | met | Checked on the netlist, sequence in D-004 |
| R-18 | Made and assembled by JLCPCB; BOM with LCSC part numbers | partial | D-014, D-016; `docs/bom.md`; 2 parts must be consigned |

## 2. Design decisions

Newest first. Each entry: context, choice, alternatives, rationale, source (datasheet section).

## D-021 — Signal tracks allowed on In1.Cu for this design (status: accepted, 2026-10-24)
Context: hand-finishing `hardware/autoroute/rp569-krt.kicad_pcb`, the last links (HSPI, inter-chip UART, CH EXT
header lines, VBUS into the TPS2121, …) could not be routed without In1. Moving those 62 segments to In2.Cu
creates 40 crossings and 12 shorts (In2 is already full there).
Choice (user): keep short signal/power tracks on In1.Cu, the GND reference plane, for this revision.
Checked: the In1 GND fill stays one piece; under the USB pairs the only reference gaps are via antipads
(1–2 mm stretches near pads), none from In1 tracks. Do not route anything else on In1 near the USB pairs.
Alternatives for a later revision: 6-layer stack, or a placement that frees In2 (see D-018–D-020).

## D-020 — Corridor between the chips kept clear: TPS2121 at the USB-C, sense dividers at their rail, pull-ups at their chip (status: accepted, 2026-10-24)
Context: of the 21 links KRT left on the D-019 board, most ran through the corridor between the RP2350 and the
CH569. The placer had put the TPS2121 (U2) and its six passives there (centroid of VBUS_C from the top and
VBUS_3 from the right edge), the CH3V3 sense divider up by the RP2350 ADC pin, and the inter-chip pull-ups
half-way along their lines.
Choice (gen_pcb.py): TPS2121 fixed at (33.5, 7.5), next to the USB-C VBUS pins; its dividers, ILIM resistor and
SS cap follow its PMUX_* pins. Sense dividers placed at the rail they measure (VBUS3 at the USB3 connector,
CH3V3 at the 3V3_CH buck inductor), so only the high-impedance sense line runs to the RP2350. A pull-up to +3V3
(RP2350 rail) sits at its RP2350 pin, one to 3V3_CH at its CH569 pin.
Rationale: frees the HSPI / control corridor; power traces stay short, the long runs are thin signal lines.

## D-019 — HSPI and Power net classes at 0.1 mm clearance (status: accepted, 2026-10-24; KRT routing under trial)
Context: KiCadRoutingTools (KRT) with QFN fanout of both chips left 13 of ~405 connections at 0.1 mm clearance
everywhere, but 32 when the 0.15 mm (HSPI, Power) and 0.13 mm (USB) class clearances were kept (Freerouting:
35). The 0.4 mm-pitch escapes do not fit at 0.15 mm.
Choice (user, 2026-10-24): Power (VBUS, 5 V, switch nodes) 0.15 → 0.1 mm, HSPI 0.15 → 0.1 mm, USB pairs keep
0.13 mm to other nets. Track widths unchanged.
Alternatives: keep every class and hand-finish ~30 links; 6-layer board.
Rationale: 0.1 mm is JLCPCB standard; nothing on the 5 V nets needs more; HSPI runs are 2–3 cm on one board,
the extra crosstalk is the accepted cost (firmware can lower drive strength / slew if needed); USB keeps margin
for its 90 Ω impedance.
Routing: `hardware/gen/route_krt.sh <workdir>` (KRT chain, documented in `docs/layout.md`); Freerouting
(`route.py`) stays as a documented option.

## D-018 — Routability: HSPI series resistors dropped, pins reassigned by chip side, buttons/LEDs at the edges (status: accepted, 2026-10-24)
Context: after D-017 the autorouter still left ~40–60 of ~420 connections; the leftovers sat in the HSPI corridor
(four 33 Ω arrays between the chips) and on the CH569 bottom/left edge. The user allowed (2026-10-24): dropping the
HSPI series resistors, reassigning the reassignable pins, moving buttons and LEDs away from the chips.
Choice:
- HSPI: RP2350 GPIO8–23 wired straight to the CH569 (2–3 cm on one board, no termination). The 4 × 4D02 33 Ω arrays
  are gone. Firmware: keep RP2350 drive strength low (4 mA) and slew limited on these pins if ringing shows up.
- RP2350 GPIO24–39 (RP2350 datasheet §1.2.3 table 3): UART1 24/25 → CH569 (F2), I2C1 26/27 link (F3),
  28 CH569_RST_N, 29 CH569_PWR_EN, 30 CH569_PWR_FAULT_N (lower right side, toward the CH569 and its power switch);
  32/33 UART0 debug (F2, top-right corner, toward RP EXT); 36/37 I2C0 breakout (F3, top side, toward RP I2C);
  LEDs on 34/35/38 (PWM9A/9B/11A); buttons on 31/39. The two UARTs and the two I2C buses stay on different instances.
- CH569 generic GPIOs (CH569DS1 table 1-2): EXT header = PA17 + bottom pins PB3, PB4, PA16, PB7, PB8, PB9, PB22,
  PB10, PB23, PB11, PB24 (header pin order = chip pin order); CH I2C (bit-bang) PB12/PB13 (bottom-right corner,
  toward the right-edge header); I2C link (bit-bang) PA14/PA15 (right side, toward the RP2350); buttons PA12/PA13;
  SD card detect PA20 (bottom-left corner, next to the socket). LEDs stay on PB0–PB2 (PWM/timer pins).
  HSPI, SD, USB, UARTs, RST# and PA5 (boot) are unchanged.
- Buttons and LEDs fixed at the board edges, each on its chip's half (gen_pcb `fixed`).
Alternatives: 6-layer board, 55×75 mm board, smaller vias, hand-finishing in pcbnew; kept for later if needed.
Rationale: removes 16 connections and 4 parts from the densest area; every reassigned line now leaves its chip on
the side facing its destination.

## D-017 — v0.3 placement and routing setup (status: accepted, 2026-10-24)
Placement from pin sides: RP2350B at (25, 19) rotated 90° (GPIO8–23 face down), CH569 at (25, 46) rotated 270°
(HSPI pins up, USB3 pins at its top-right corner next to the USB3 connector). Bottom keep-out = QFN pad box + 1.2 mm,
0.4 mm part spacing, passives turned toward their net partners.
Net classes: `Rail` 0.2/0.1 mm for +3V3, 3V3_CH, 1V2_CH, 1V1 and GND (Freerouting cannot neck a 0.3 mm track into
the 0.2 mm QFN pins); `Power` 0.3 mm only for VBUS / 5 V / switch nodes (clearance 0.1 mm since D-019). Hole-to-copper clearance 0.2 mm.
Routing (`hardware/gen/route.py`): In1 is the GND plane; Freerouting places no via for a plane net, so the script
puts a locked via beside every SMD GND pad (and 4 in each exposed pad) before export. Freerouting 2.1.0 is used
patched so that `-mp` works (`hardware/gen/patch_freerouting_mp.py`): the stock jar ignores it and its job
timeout stops mid-pass.

## D-016 — Stackup JLC04161H-7628 and JLCPCB design rules; autorouting with Freerouting (status: proposed, 2026-10-23)
Stackup: JLCPCB 4-layer 1.6 mm JLC04161H-7628 (L1–L2 prepreg 7628, 0.2104 mm, er 4.4). In1/In2 = GND planes.
USB3/USB2 90 Ω differential on F.Cu over In1: 0.24 mm / 0.13 mm gap (IPC-2141A estimate ~91 Ω). **Confirm with
JLCPCB's impedance calculator**, or order controlled impedance. Rules: track/clearance 0.15/0.1 mm default, power
0.3 mm, vias 0.3/0.5 mm (JLCPCB standard, no extra cost).
Routing: `hardware/gen/route.py` (Freerouting 2.1.0, Java 21) routes the generated board. Freerouting does not
route coupled differential pairs, so the USB pairs need a manual check / re-route in pcbnew.

## D-015 — Power OR = TI TPS2121 instead of 2 × LM66100 (status: accepted, 2026-10-23)
Context: LM66100 is not stocked by JLCPCB (D-014). User chose the TPS2121 (C485916, 28k in stock).
Choice: TPS2121RUXR in VCOMP mode (datasheet SLVSEA3F §10.5): IN1 = VBUS_C, IN2 = VBUS_3, OUT = 5V_SYS; PR1 = CP2 =
GND, so the higher input wins (IN2 on a tie); OV1/OV2 dividers 23.7k/4.99k → inputs above 1.06 × 28.69/4.99 ≈ 6.1 V
are rejected; RILM 100k (range 18–100k) ≈ 1.2 A typical, load < 0.3 A; SS 10 nF (TI example); ST tied to GND.
Symbol and footprint (RUX0012A land pattern) generated by `gen/mk_lib.py` and `gen/mk_fp.py` from the datasheet.

## D-014 — JLCPCB assembly, LCSC part numbers in board.py (status: accepted, 2026-10-23)
Requirement R-18. Choice: every part gets an LCSC number from the JLCPCB catalogue (jlcparts snapshot 2026-10-23),
preferring Basic parts and large stock. `gen_bom.py` writes JLCPCB BOM + CPL files to `hardware/jlcpcb/`.
Substitutions made for availability: RGB LED → LiteOn LTST-C19HE1WT (G/B resistors 100 Ω, D-005); 2.2 µH →
FTC303020D2R2MBCA; LM66100 ×2 → TPS2121 (D-015). Not at JLCPCB, consigned: CH569W, GSB343K33HR. Details: docs/bom.md.

## D-013 — USB3 Micro-B = Amphenol GSB343K33HR, through-hole shell legs (status: accepted, 2026-10-23)
Requirement R-16: through-hole mounting legs for a firm mechanical hold. The Connfly DS1104-01 (D-008) is all-SMD
and is the only USB3 Micro-B footprint in the KiCad 10 library.
Choice: Amphenol GSB343K33HR (10 SMD contacts at 0.65 mm, 2 shell DIP legs, L = 1.0 mm; the `1` variant has 1.4 mm
tails for thicker boards). Footprint `rp569:USB3_Micro-B_Amphenol_GSB343K33HR`, generated by `hardware/gen/mk_fp.py`
from the drawing's recommended PCB layout: pads 0.40 × 1.35 mm, slots 0.50 × 1.30 mm at 12.55 mm, slot centre line
1.45 mm from the PCB edge. The annular ring (0.25 mm) is my choice, the drawing does not give it. Shell legs on GND.
Source: datasheet/GSB343K33HR/gsb343k33hr.pdf (rev C), product spec ps-7428.pdf. Other candidate: ATTEND 209E-BE01.

## D-012 — Debug UART of the RP2350 moved onto the RP EXT header (status: proposed)
Context: 50×70 mm leaves no edge room for a separate 3-pin RP UART header. GPIO30/31 became user buttons.
Choice: RP EXT 1x10 = 3V3, DBG_TX (GPIO32), DBG_RX (GPIO33), ADC0–5, GND. CH EXT shrinks to 2x07 (PB3/PB4 → CH buttons).

## D-011 — Board 50×70 mm, two-sided, chamfered corners (status: accepted, 2026-10-23)
Requested by user: 50×70 mm, angled corners, parts on both sides, main chips on the same side as the
USB connectors and the microSD. Choice: 3 mm 45° chamfers; RP2350/USB-C top half, CH569/USB3/microSD bottom
half, all on F.Cu; small passives and the flash on B.Cu; USB ESD and SSTX caps forced to F.Cu.
Also added on request: CH569 boot jumper (2-pin 2.54 mm, PA5 → GND via 1k), 2 user buttons per chip,
RGB LEDs placed next to their chip.

## D-010 — KiCad files generated from a Python netlist (status: accepted)
Choice: hardware/gen/board.py is the single source; gen_sch/gen_pcb/gen_pro produce the KiCad files; build.sh runs ERC/DRC+parity.
Rule: once routing starts, gen_pcb.py must no longer be run (it would wipe the routing). Use Update PCB from Schematic. See docs/layout.md.

## D-009 — CH569 load switch = AP2171W instead of TPS2553 (status: proposed)
Context: TPS2553 is not in the KiCad standard library; AP2171W is (SOT-23-5, 1 A current limit, active-high EN, FLG# open-drain).
Choice: AP2171W. Check that 1 A covers the CH569 + SD card peak current.

## D-008 — USB3 connector = USB3 Micro-B (Connfly DS1104-01) (status: superseded by D-013)
Context: KiCad lib has no USB3 Type-B footprint. A USB-C SuperSpeed port would need an orientation mux.
Choice: USB3 Micro-B receptacle (device), SSTX AC caps 100 nF, TPD2EUSB30 ESD on all 3 pairs.
Alternative: USB3 Type-A plug like phantomdrive (stick form factor).

## D-007 — Power paths: ideal-diode OR for 5V_SYS, switched 5V_CH from USB3 VBUS (status: proposed)
Choice: VBUS_C and VBUS_3 ORed (first two LM66100 ideal diodes, now one TPS2121, D-015) → 5V_SYS (RP2350 side, no back-feed between connectors).
CH569 powered only from VBUS_3 through a current-limited load switch (AP2171W, D-009), EN = RP2350 GPIO35 (pull-down), FAULT# → GPIO39.
Optional DNP link 5V_SYS → switch input to run the CH569 without a USB3 cable.
Requested by user 2026-10-23. Details: docs/power.md.

## D-006 — CH569 power follows phantomdrive (status: proposed)
Choice: dedicated 3V3 and 1V2 TLV62569 bucks for the CH569 (external 1.2 V on V12CORE/V12USB), fed from switched 5V_CH (D-007).
Source: docs/ch569-hardware-notes.md.

## D-005 — Discrete RGB LEDs on PWM pins (status: proposed)
Choice: one discrete RGB LED per chip. RP2350 GPIO34/35/38 (PWM9A/9B/11A, D-018); CH569 PB0 (TMR1_), PB1 (PWM2), PB2 (PWM3).
Part (D-014): LiteOn LTST-C19HE1WT, common anode to the chip's 3.3 V rail, GPIO sinks; 330 Ω red, 100 Ω green/blue.
Alternatives: WS2812/SK6812 (1 pin, but 5 V supply/levels and a timing-critical protocol on the CH569).
Rationale: works at 3.3 V on both chips, PWM in hardware on both.

## D-004 — CH569 bootloader entry and reflashing driven by the RP2350 (status: accepted, checked 2026-10-23)
Context: automated CH569 flashing driven by the RP2350. RST# (PB15) is disabled by default (RESET_EN = 0), so
the CH569 only samples its boot pin PA5 at power-up. PA5 is also HSPI HD0.
Choice: the RP2350 does in firmware what the BOOT button does by hand:
1. GPIO8 (HSPI_D0_RP) drives low. It reaches PA5 through a 33 Ω array element (RN3). The BOOT button and the
   boot jumper pull the same node low through 1 kΩ (R33), so the RP2350 pulls harder than the button.
2. GPIO35 (CH569_PWR_EN) goes low: the AP2171W turns 5V_CH off, both CH569 bucks stop. GPIO47 (CH3V3_SENSE)
   confirms 3V3_CH has gone down.
3. GPIO35 goes high: the CH569 powers up with PA5 low and starts the USB bootloader (`4348:55E0`) on the
   USB3 connector's USB2 lines. The host flashes with `wch-ch56x-isp` within ~10 s.
4. GPIO8 is released and becomes HSPI D0 again.
Conditions: VBUS_3 present (the USB3 cable is needed for flashing anyway, or fit the DNP 0 Ω from 5V_SYS).
Driving GPIO8 low while the CH569 is unpowered is safe (no back-power through a low level).
Conflict cases: jumper fitted or button held while GPIO8 drives high → 3.2 mA through 1 kΩ + 33 Ω, harmless,
but the RP2350 wins and the CH569 boots its application. Firmware must leave GPIO8 low or Hi-Z at CH569 power-up
when the user wants the jumper to take effect. A blank RP2350 leaves GPIO8 as an input with its default pull-down,
which does not fight the jumper.
RST# stays wired to GPIO34 (open-drain, 10k to 3V3_CH) for when RESET_EN is set in the CH569 config.
Alternatives: 2-wire debug from the RP2350 (TCK/TIO share pins with HTACK/HTCLK, protocol not public).
Source: CH569DS1 pin table (pin 15 PA5, pin 30 PB15) and config bits §2; project/hydrausb3/wch-ch56x-isp/README.md
(boot pin PA5, alternative PA13, 10 s timeout); phantomdrive SW1 on PA5. Netlist trace: hardware/gen/board.py.

## D-003 — Parallel bus width vs SD card pin conflict (status: accepted — option (a), 2026-10-23)
Context: CH569DS1 pin table: HSPI HD4..HD7 = PA2,PA1,PA0,PB21 (= eMMC MD7..MD4),
HD8..HD11 = PB20..PB17 (= MD3..MD0), HD13 = PB16 (= MCMD). A 4-bit SD card needs MD0..MD3 + MCMD + clock.
Consequence: 16-bit HSPI and an SD card on the eMMC controller cannot coexist. 8-bit HSPI (HD0..HD7) + 4-bit SD can.
Options: (a) 8-bit HSPI + 4-bit SD on eMMC controller (fits the 8-channel logic analyser naturally);
(b) 16-bit HSPI + SD in SPI mode on CH569 SPI0/SPI1 (slow, ~few MB/s);
(c) 16-bit HSPI, no SD card.
Decision: (a) 8-bit HSPI on HD0..HD7 + 4-bit SD card on the eMMC controller (MD0..MD3, MCMD, clock).
HD8..HD31 and MD4..MD7 are not used as such and stay free for GPIO (MD4..MD7 = HD4..HD7 are taken by HSPI).
Source: datasheet/CH569DS1.pdf, pin description table (pins 15–29, QFN68). Verify in the PDF.

## D-002 — RP2350 → CH569 data link uses CH569 HSPI in 8-bit mode (status: accepted, width changed by D-003)
Context: Need high-throughput RP2350 → CH569 → USB3 path.
Choice: RP2350 PIO emulates an HSPI transmitter (8 data HD0..HD7 + clock + handshake lines).
Alternatives: SerDes (not reachable from RP2350), SPI (too slow).
Open: exact HSPI handshake, max clock achievable with RP2350 PIO + board routing.

## D-001 — Separate crystals per chip (status: proposed)
Context: User wants a clock shared between RP2350B and CH569.
Choice: Each chip keeps its own reference crystal (CH569 30 MHz, RP2350 12 MHz). The shared
clock is the parallel-bus data clock, driven by RP2350 PIO toward CH569 HSPI.
Rationale: CH569 USB3 PLL has tight jitter requirements; a GPIO-generated clock is a risk.
