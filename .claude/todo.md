# TODO

## Open design questions (verify against datasheets)

- [ ] CH569 eMMC controller with an SD card: confirm SD (not only eMMC) is supported in 4-bit
      mode (init sequence, 3.3 V signalling; no UHS 1.8 V switching).
- [ ] CH569 I2C: datasheet only mentions I2C in the DVP/sensor context, no I2C peripheral found → bit-banged. Confirm.
- [ ] Logic analyser: 8 RP2350B inputs sampled by PIO → parallel bus. Pick sample rate target, input protection/level (3.3 V, series R), header.
- [x] ~~CH569 programming path~~ → D-004 proposed (power-cycle + PA5). Original options: — pick one (see specification.md):
      (a) RP2350 drives RESET# + boot strap, host flashes via USB ISP (`wchisp`);
      (b) UART ISP driven by RP2350 (only if CH569 bootloader supports UART);
      (c) RP2350 acts as 2-wire debug probe (RVSWD-like) — needs protocol research.
- [ ] HSPI framing: HSPI is a packetised protocol (header, valid/req/ack lines, CRC options).
      Confirm the exact handshake so RP2350 PIO can emulate the TX side; bus width is 8-bit (D-003); pick clock.
- [ ] Clocks: CH569 30 MHz crystal (own, for USB3 PLL), RP2350 12 MHz crystal (own).
      The shared "clock" is the HSPI data clock driven by RP2350 — not a system clock.
- [ ] CH569 power rails (3.3 V + core rail?) and USB3 connector choice (USB-C SS w/ mux vs USB-B 3.0 / USB-C single orientation).
- [ ] RP2350B GPIO budget: 8 data + clk + ~3 ctrl + UART(2) + shared I2C(2) + breakout I2C(2) + RESET/BOOT(2)
      + logic analyser inputs(8) + RGB LED (1 if WS2812-type, 3 if discrete) + USB, SWD, QSPI flash.
- [ ] RGB LED type per chip: addressable (WS2812/SK6812, 1 pin, 5 V-ish timing) vs discrete RGB (3 PWM pins). CH569 side: check free GPIO/PWM.
- [ ] CH569 breakout I2C: pick 2 free GPIOs (bit-bang), not colliding with HSPI/eMMC/UART.

## Tasks


- [ ] Power (D-007): confirm parts (LM66100, TPS2553, TLV62569) and stock; size the TPS2553 current limit from the CH569 current draw.
- [ ] Check that the CH569 I/Os don't clamp the shared I2C / inter-chip lines while the CH569 is unpowered (power.md rules).
- [ ] Review `docs/pinmap.md` v0.2 (D-018 reassignment) with the user, then freeze it.
- [ ] Confirm the HSPI packet format (header/CRC, Table 10-x) so the PIO TX program can be written.
- [ ] Read HydraUSB3 schematic + `HydraUSB3_V1_CH569_HSPI.ods` to answer the HSPI/eMMC pin-collision and power-rail questions above.
- [ ] Collect datasheets/reference designs into `docs/` (CH569 EVT schematic, RP2350 hardware design guide).
- [ ] Block diagram (`docs/`).
- [ ] Review schematic v0.1 (`hardware/rp569-schematic.pdf`) with the user.
- [ ] Confirm the 90 Ω width/gap (0.24/0.13 mm, D-016) with JLCPCB's impedance calculator.
- [ ] Hand-review v0.3 placement (RP2350 VREG loop, buck loops, USB3 caps/ESD, crystals). See `docs/layout.md`.
- [ ] Routing: finish `hardware/autoroute/rp569-krt.kicad_pcb` (21 links left, 6 dangling tracks + 4 vias to delete) by hand in pcbnew; re-route the 3 uncoupled USB pairs with the diff-pair router; HSPI length match.
- [ ] Place the USB3 ESD (TPD2EUSB30) in line with each pair so the pairs can stay coupled.
- [ ] Silkscreen cleanup.
- [ ] Consigned parts: CH569W and GSB343K33HR (LM66100 replaced by TPS2121, D-015).
- [ ] Check JLCPCB rotations in the placement preview; recheck stock before ordering.
- [ ] Confirm the 3.3 µH alternative against the RP2350 HW design guide (AOTA-B201610S3R3 has 119 in stock).
- [ ] Check AP2171W 1 A limit vs. CH569 + SD peak current (D-009).
