# BOM for JLCPCB assembly

Status: **v1.0, 2026-10-24.** Production files in `hardware/jlcpcb/`: `rp569-gerbers.zip` (board), `rp569-bom.csv`
(BOM) and `rp569-cpl.csv` (placement, from the routed board `hardware/rp569-routed.kicad_pcb`), written by
`hardware/gen/gen_fab.py` (DRC gate, gerbers, drill, then `gen_bom.py`; `build.sh` also re-runs `gen_bom.py`). LCSC numbers live in
`hardware/gen/board.py` (`LCSC_BY_VALUE`) and are copied into the `LCSC` and `MPN` fields of every symbol and
footprint.

Stock, class and price come from the [jlcparts](https://github.com/yaqwsx/jlcparts) snapshot of the JLCPCB
catalogue dated 2026-10-23. **Check stock again on the JLCPCB site when ordering.**

Summary: see the table below; 2 consigned parts (CH569W, GSB343K33HR). 2026-10-24: the four HSPI 33 Ω arrays
(C25501, Extended) were dropped (D-018), one Extended line and its loading fee fewer. The LM66100 pair was replaced by a TI TPS2121 (C485916, D-015).
Parts cost about $15.5 per board at quantity 1, excluding the consigned parts and assembly fees.

## Parts JLCPCB does not stock (consign or hand-solder)

| Part | Refs | Why it stays | Options |
|---|---|---|---|
| WCH CH569W (QFN-68) | U10 | The whole point of the board; JLCPCB lists no CH569/CH565 | Buy from WCH/LCSC marketplace/AliExpress, send as consigned part |
| Amphenol GSB343K33HR | J10 | Through-hole shell legs (R-16) | Consign; JLCPCB only has the SMT-shell GSB343133HR (C464578, 28 in stock) |

## Changes made for JLCPCB

- RGB LEDs: Cree PLCC4 footprint replaced by **LiteOn LTST-C19HE1WT** (C458749, 1.6×1.6 mm, common anode, KiCad
  footprint exists). Pinout from its datasheet: 1 = red K, 2 = green K, 3 = blue K, 4 = common anode; symbol
  `Device:LED_RGBA` has the same numbering. Its green/blue Vf is 2.8–3.9 V on a 3.3 V rail, so G/B series
  resistors went from 330 Ω to 100 Ω (red stays 330 Ω).
- 2.2 µH buck inductors: FTC303015D2R2MBCA had only 9 in stock → **FTC303020D2R2MBCA** (C7423318, same Changjiang
  3×3 mm family, 2.0 mm high instead of 1.5 mm, 4.7 A).
- 3.3 µH RP2350 inductor: **AOTA-B201610S3R3-101-T** (C42411119), the part named in the RP2350 hardware design
  guide. Only 119 in stock: order early or keep an alternative (e.g. FTC201612S3R3MBCA C18724415, not yet
  checked against the RP2350 guide's requirements).
- 12 MHz crystal: **ABM8-272-T3** (C20625731), the RP2350 hardware design guide part (CL 10 pF, 15 pF caps).
- 30 MHz crystal: X322530MOB4SI (C70592), CL 12 pF; 15 pF load caps kept as in phantomdrive.

## Before ordering

- Check every rotation in JLCPCB's placement preview: KiCad and JLCPCB disagree for some packages
  (QFNs, SOT-23, connectors, LEDs).
- Pin headers are THT. JLCPCB fits them only with THT assembly; otherwise remove those lines and hand-solder.
- The CPL comes from `kicad-cli pcb export pos` (Y is negative, as JLCPCB expects for KiCad files).

## Full BOM

44 lines: 18 Basic, 24 Extended, 2 consigned; parts ≈ $16.45 per board (qty 1).

| Qty | Value | MPN | LCSC | Class | JLC stock | $/pc (1+) | Refs |
|---:|---|---|---|---|---:|---:|---|
| 1 | USB-C (RP2350) | TYPE-C-31-M-12 | C165948 | Extended | 367975 | 0.1872 | J1 |
| 2 | 5.1k | 0402WGF5101TCE | C25905 | Basic | 5311748 | 0.0024 | R1,R2 |
| 4 | TPD2EUSB30 | TPD2EUSB30DRTR | C97502 | Extended | 7511 | 0.6852 | U1,U10,U11,U12 |
| 6 | 4.7uF | CL05A475MP5NRNC | C23733 | Basic | 1858838 | 0.0168 | C1,C29,C30,C31,C49,C51 |
| 1 | TPS2121 | TPS2121RUXR | C485916 | Extended | 27903 | 0.9196 | U2 |
| 2 | 23.7k | AC0402FR-0723K7L | C226363 | Extended | 19167 | 0.0046 | R3,R5 |
| 2 | 4.99k | 0402WGF4991TCE | C25903 | Extended | 277619 | 0.0021 | R4,R6 |
| 10 | 100k | 0402WGF1003TCE | C25741 | Basic | 9721665 | 0.0015 | R7,R10,R13,R15,R16,R17,R18,R20,R21,R51 |
| 1 | 10nF | CL05B103KB5NNNC | C15195 | Basic | 6322666 | 0.0041 | C2 |
| 6 | 10uF | CL21A106KAYNNNE | C15850 | Basic | 3710191 | 0.0692 | C3,C5,C6,C9,C12,C32 |
| 1 | 0R | 0402WGF0000TCE | C17168 | Basic | 9136350 | 0.0011 | R8 |
| 1 | AP2171W | AP2171WG-7 | C110466 | Extended | 14896 | 0.1943 | U3 |
| 1 | 1uF | CL05A105KA5NQNC | C52923 | Basic | 7514754 | 0.0081 | C4 |
| 7 | 10k | 0402WGF1002TCE | C25744 | Basic | 21489602 | 0.0033 | R11,R27,R33,R34,R37,R39,R40 |
| 3 | TLV62569 | TLV62569DBVR | C141836 | Extended | 460463 | 0.0709 | U4,U5,U6 |
| 3 | 2.2uH | FTC303020D2R2MBCA | C7423318 | Extended | 18650 | 0.2584 | L1,L2,L3 |
| 2 | 453k | 0402WGF4533TCE | C27009 | Extended | 281203 | 0.0024 | R12,R14 |
| 6 | 22uF | CL21A226MAQNNNE | C45783 | Basic | 3360923 | 0.2384 | C7,C8,C10,C11,C13,C14 |
| 6 | 47k | 0402WGF4702TCE | C25792 | Basic | 5135462 | 0.0026 | R19,R46,R47,R48,R49,R50 |
| 1 | RP2350B | RP2350B | C42415655 | Extended | 2925 | 1.29 | U7 |
| 27 | 100nF | CL05B104KO5NNNC | C1525 | Basic | 22560770 | 0.0043 | C15,C16,C17,C18,C19,C20,C21,C22,C23,C24,C25,C26,C27,C28,C35,C36,C37,C38,C39,C40,C41,C42,C43,C44,C47,C48,C50 |
| 1 | 3.3uH | AOTA-B201610S3R3-101-T | C42411119 | Extended | 119 | 0.2853 | L4 |
| 1 | 33R | 0402WGF330JTCE | C25105 | Basic | 1399949 | 0.0012 | R22 |
| 1 | 12MHz | ABM8-272-T3 | C20625731 | Extended | 10637 | 0.6245 | Y1 |
| 3 | 1k | 0402WGF1001TCE | C11702 | Basic | 14385321 | 0.0036 | R23,R26,R38 |
| 4 | 15pF | 0402CG150J500NT | C1548 | Basic | 1214691 | 0.0038 | C33,C34,C45,C46 |
| 2 | 27R | RC0402FR-0727RL | C138021 | Extended | 58304 | 0.004 | R24,R25 |
| 1 | W25Q128JVS | W25Q128JVSIQ | C97521 | Basic | 66921 | 2.5734 | U8 |
| 7 | PTS810SJG250SMTRLFS | PTS810SJG250SMTRLFS | C221895 | Extended | 12008 | 0.3422 | SW1,SW2,SW3,SW4,SW5,SW6,SW7 |
| 2 | 2.54-1*3P | 2.54-1*3P | C49257 | Extended | 253882 | 0.0284 | J2,J8 |
| 2 | RGB | LTST-C19HE1WT | C458749 | Extended | 80597 | 0.0733 | D1,D2 |
| 2 | 330R | 0402WGF3300TCE | C25104 | Basic | 1355581 | 0.0041 | R28,R41 |
| 4 | 100R | 0402WGF1000TCE | C25076 | Basic | 8287966 | 0.0012 | R29,R30,R42,R43 |
| 2 | PZ254V-11-04P | PZ254V-11-04P | C492403 | Extended | 57456 | 0.0284 | J3,J7 |
| 6 | 4.7k | 0402WGF4701TCE | C25900 | Basic | 16063321 | 0.0023 | R31,R32,R35,R36,R44,R45 |
| 2 | PZ254V-11-10P | PZ254V-11-10P | C492409 | Extended | 45897 | 0.0609 | J4,J5 |
| 2 | 100R | 4D02WGJ0101TCE | C48442 | Extended | 272629 | 0.0055 | RN1,RN2 |
| 1 | CH569W | CH569W | — | **consign** | — | — | U9 |
| 1 | 30MHz | X322530MOB4SI | C70592 | Extended | 7838 | 0.101 | Y2 |
| 1 | CH BOOT JP | PZ254V-11-02P | C492401 | Extended | 1088237 | 0.0187 | J6 |
| 1 | CH EXT | PZ254V-12-14P | C492424 | Extended | 33667 | 0.0932 | J9 |
| 1 | USB3 Micro-B (CH569) | GSB343K33HR | — | **consign** | — | — | J10 |
| 1 | microSD | DM3AT-SF-PEJM5 | C114218 | Extended | 16676 | 1.3966 | J11 |
