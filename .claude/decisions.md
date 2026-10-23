# Design decisions

<!-- Format:
## D-NNN — title (status: proposed | accepted | superseded)
Context / Choice / Alternatives / Rationale / Source (datasheet section)
-->

## D-004 — CH569 reset/programming by power-cycle + boot pin (status: proposed)
Context: Automated CH569 flashing driven by RP2350. RST# (PB15) is disabled by default (RESET_EN=0);
boot pin is PA5 = HSPI HD0, already driven by RP2350 GPIO8.
Choice: RP2350 GPIO35 gates the CH569 power rails (CH569_PWR_EN on the bucks' EN pins). To flash: hold GPIO8 low,
power-cycle the CH569, the host flashes over the USB3 connector's USB2 lines with `wch-ch56x-isp`. RST# is still
wired to GPIO34 (open-drain) for use when RESET_EN is set.
Alternatives: 2-wire debug from RP2350 (TCK/TIO share pins with HTACK/HTCLK; protocol not public).
Source: CH569DS1 Table 1-2 (pins 15, 30), §2 config bits; project/hydrausb3/wch-ch56x-isp/README.md; phantomdrive SW1 on PA5.

## D-005 — Discrete RGB LEDs on PWM pins (status: proposed)
Choice: one discrete RGB LED per chip. RP2350 GPIO36/37/38 (PWM10A/10B/11A); CH569 PB0 (TMR1_), PB1 (PWM2), PB2 (PWM3).
Alternatives: WS2812/SK6812 (1 pin, but 5 V supply/levels and a timing-critical protocol on the CH569).
Rationale: works at 3.3 V on both chips, PWM in hardware on both.

## D-010 — KiCad files generated from a Python netlist (status: accepted)
Choice: hardware/gen/board.py is the single source; gen_sch/gen_pcb/gen_pro produce the KiCad files; build.sh runs ERC/DRC+parity.
Rule: once routing starts, gen_pcb.py must no longer be run (it would wipe the routing). Use Update PCB from Schematic. See docs/layout.md.

## D-009 — CH569 load switch = AP2171W instead of TPS2553 (status: proposed)
Context: TPS2553 is not in the KiCad standard library; AP2171W is (SOT-23-5, 1 A current limit, active-high EN, FLG# open-drain).
Choice: AP2171W. Check that 1 A covers the CH569 + SD card peak current.

## D-008 — USB3 connector = USB3 Micro-B (Connfly DS1104-01) (status: proposed)
Context: KiCad lib has no USB3 Type-B footprint. A USB-C SuperSpeed port would need an orientation mux.
Choice: USB3 Micro-B receptacle (device), SSTX AC caps 100 nF, TPD2EUSB30 ESD on all 3 pairs.
Alternative: USB3 Type-A plug like phantomdrive (stick form factor).

## D-007 — Power paths: ideal-diode OR for 5V_SYS, switched 5V_CH from USB3 VBUS (status: proposed)
Choice: VBUS_C and VBUS_3 ORed by two LM66100 ideal diodes → 5V_SYS (RP2350 side, no back-feed between connectors).
CH569 powered only from VBUS_3 through a TPS2553 current-limited load switch, EN = RP2350 GPIO35 (pull-down), FAULT# → GPIO39.
Optional DNP link 5V_SYS → switch input to run the CH569 without a USB3 cable.
Requested by user 2026-10-23. Details: docs/power.md.

## D-006 — CH569 power follows phantomdrive (status: proposed)
Choice: dedicated 3V3 and 1V2 TLV62569 bucks for the CH569 (external 1.2 V on V12CORE/V12USB), fed from switched 5V_CH (D-007).
Source: docs/ch569-hardware-notes.md.

## D-001 — Separate crystals per chip (status: proposed)
Context: User wants a clock shared between RP2350B and CH569.
Choice: Each chip keeps its own reference crystal (CH569 30 MHz, RP2350 12 MHz). The shared
clock is the parallel-bus data clock, driven by RP2350 PIO toward CH569 HSPI.
Rationale: CH569 USB3 PLL has tight jitter requirements; a GPIO-generated clock is a risk.

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
