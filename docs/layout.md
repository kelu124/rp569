# Layout status

Status: **v0.1, 2026-10-23. Schematic complete, PCB placed, NOT routed.**

![PCB render](img/pcb-render-v0.1.png)

## How the KiCad files are produced

Everything under `hardware/` is generated from `hardware/gen/board.py` (parts, footprints, pin→net map):

| Script | Output |
|---|---|
| `gen/mk_lib.py` | `lib/rp569.kicad_sym`: CH569W symbol from phantomdrive (MIT), pin types corrected |
| `gen/gen_sch.py` | `rp569.kicad_sch`: one sheet, symbols grouped by block, a net label on every pin |
| `gen/gen_pcb.py` | `rp569.kicad_pcb`: 4-layer 100×70 mm, footprints + nets, edge connectors, automatic placement, GND planes on In1/In2 |
| `gen/gen_pro.py` | net classes in `rp569.kicad_pro` (USB3_90R, USB2_90R, HSPI, Power) |
| `gen/build.sh` | runs all of the above, then ERC and DRC with schematic parity → `hardware/reports/` |

**Once hand-routing starts, stop running `gen_pcb.py`**: it rebuilds the board from scratch and would
wipe the routing. From then on, change `board.py` → `gen_sch.py` → in pcbnew *Tools → Update PCB from
Schematic*. Footprints are already linked to their symbols, and schematic parity is 0 issues.

## Check results (v0.1)

- ERC: 1 error, expected (two LM66100 outputs on 5V_SYS = ideal-diode OR).
- DRC: 0 electrical violations; ~280 silkscreen overlaps (cosmetic, to clean up); 394 unrouted connections.
- Schematic ↔ PCB parity: 0 issues.

## Placement summary

- Left edge: USB-C (RP2350), the 3V3 buck, and the ideal diode for VBUS_C.
- Center-left: RP2350B with decoupling caps placed beside their pins, plus flash, 12 MHz crystal, BOOTSEL/RUN.
- Center: 33 Ω series arrays for the HSPI bus.
- Center-right: CH569W with decoupling, 30 MHz crystal, BOOT button, RGB LED.
- Right edge: USB3 Micro-B, USB3 ESD, the CH569 power path (ideal diode, AP2171W switch, 3V3/1V2 bucks).
- Bottom-right: microSD. Top edge: LA, RP EXT, SWD, RP I2C, RP UART headers. Bottom edge: CH I2C,
  CH UART, CH EXT headers.

## To do before routing

1. **Stackup**: choose the fab (e.g. JLCPCB 4-layer JLC04161H-7628) and recompute the 90 Ω differential
   width/gap for USB3_90R / USB2_90R (placeholders are 0.15/0.15 mm).
2. Review the placement by hand. These were placed by an algorithm, not by engineering judgement:
   - RP2350 VREG loop: VREG_VIN cap, inductor and DVDD cap tight together (RP2350 datasheet,
     "External components and PCB layout requirements").
   - TLV62569 loops: CIN, SW node and inductor compact.
   - SuperSpeed AC caps and ESD close to the USB3 connector.
   - Crystals right next to their chips.
3. Check the LED footprint pinout (`LED_Cree-PLCC4_2x2mm_CW`) against the RGB LED actually bought (pin 1 = anode).
4. Silkscreen cleanup.

## Routing order (suggested)

1. USB3 SuperSpeed pairs (SSTX via AC caps, SSRX) and USB2 pairs: hand-routed on F.Cu over the In1 GND
   plane, length-matched within each pair, no vias if possible.
2. Power: VBUS → ideal diodes → bucks; RP2350 VREG; CH569 1V2/3V3 pours.
3. HSPI bus (16 lines through the 33 Ω arrays): matched lengths ±1 mm relative to HSPI_RCLK.
4. Everything else. Freerouting (not installed) could autoroute the low-speed nets after steps 1–3.
