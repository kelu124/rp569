# TODO

## Open design questions (verify against datasheets)

- [ ] CH569 pin muxing: confirm HSPI 16-bit data/clock pins do **not** collide with the
      eMMC/SD controller pins on CH569W QFN-68.
- [ ] CH569 eMMC controller with an SD card: confirm SD (not only eMMC) is supported in 4-bit
      mode (init sequence, 3.3 V signalling; no UHS 1.8 V switching).
- [ ] CH569 I2C: no hardware I2C peripheral expected → bit-banged I2C on GPIO. Confirm.
- [ ] CH569 programming path for automation — pick one (see decisions.md):
      (a) RP2350 drives RESET# + boot strap, host flashes via USB ISP (`wchisp`);
      (b) UART ISP driven by RP2350 (only if CH569 bootloader supports UART);
      (c) RP2350 acts as 2-wire debug probe (RVSWD-like) — needs protocol research.
- [ ] HSPI framing: HSPI is a packetised protocol (header, valid/req/ack lines, CRC options).
      Confirm the exact handshake so RP2350 PIO can emulate the TX side; pick bus width (16-bit) and clock.
- [ ] Clocks: CH569 30 MHz crystal (own, for USB3 PLL), RP2350 12 MHz crystal (own).
      The shared "clock" is the HSPI data clock driven by RP2350 — not a system clock.
- [ ] CH569 power rails (3.3 V + core rail?) and USB3 connector choice (USB-C SS w/ mux vs USB-B 3.0 / USB-C single orientation).
- [ ] RP2350B GPIO budget: 16 data + clk + ~3 ctrl + UART(2) + shared I2C(2) + breakout I2C(2) + RESET/BOOT(2)
      + RGB LED (1 if WS2812-type, 3 if discrete) + USB, SWD, QSPI flash.
- [ ] RGB LED type per chip: addressable (WS2812/SK6812, 1 pin, 5 V-ish timing) vs discrete RGB (3 PWM pins). CH569 side: check free GPIO/PWM.
- [ ] CH569 breakout I2C: pick 2 free GPIOs (bit-bang), not colliding with HSPI/eMMC/UART.

## Tasks

- [ ] Get CH569 datasheet (CH569DS1) into `datasheet/` — WCH site is a JS app, direct link not found yet.
- [ ] Download RP2350 datasheet + hardware design guide into `datasheet/` (PDF + markitdown .md).
- [ ] Read HydraUSB3 schematic + `HydraUSB3_V1_CH569_HSPI.ods` to answer the HSPI/eMMC pin-collision and power-rail questions above.
- [ ] Collect datasheets/reference designs into `docs/` (CH569 EVT schematic, RP2350 hardware design guide).
- [ ] Block diagram + pin map spreadsheet (`docs/pinmap.md`).
- [ ] Create KiCad project in `hardware/`.
- [ ] Schematic: power, RP2350B core, CH569 core, USB3, USB-C, SD, inter-chip bus, debug headers.
- [ ] Layout: stackup (4-layer for USB3 90 Ω diff pairs), placement, routing.
