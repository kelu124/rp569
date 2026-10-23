# TODO

## Open design questions (verify against datasheets)

- [ ] Verify D-003 pin mapping (HD0..HD7, MD0..MD3, MCMD, MCK) in `datasheet/CH569DS1.pdf`, not only the .md.
- [ ] CH569 eMMC controller with an SD card: confirm SD (not only eMMC) is supported in 4-bit
      mode (init sequence, 3.3 V signalling; no UHS 1.8 V switching).
- [ ] CH569 I2C: datasheet only mentions I2C in the DVP/sensor context, no I2C peripheral found → bit-banged. Confirm.
- [ ] Logic analyser: 8 RP2350B inputs sampled by PIO → parallel bus. Pick sample rate target, input protection/level (3.3 V, series R), header.
- [ ] CH569 programming path for automation — pick one (see decisions.md):
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

- [ ] Study phantomdrive KiCad (`project/phantomdrive/ee/ovrdrive/`): CH569 power, crystal, USB3, SD wiring.
- [ ] Read HydraUSB3 schematic + `HydraUSB3_V1_CH569_HSPI.ods` to answer the HSPI/eMMC pin-collision and power-rail questions above.
- [ ] Collect datasheets/reference designs into `docs/` (CH569 EVT schematic, RP2350 hardware design guide).
- [ ] Block diagram + pin map spreadsheet (`docs/pinmap.md`).
- [ ] Create KiCad project in `hardware/`.
- [ ] Schematic: power, RP2350B core, CH569 core, USB3, USB-C, SD, inter-chip bus, debug headers.
- [ ] Layout: stackup (4-layer for USB3 90 Ω diff pairs), placement, routing.
