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

- [ ] Enable GitHub Pages (Settings → Pages → Deploy from branch `main`, folder `/docs`) and check the site renders.

- [ ] Review and publish the Hackaday drafts (`docs/hadposts/`), add a before/after ratsnest screenshot.


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
- [ ] In1 GND plane in 3 pieces at the bottom left (DRC-clean, stitched): move SD_D1 / CH_PB11 (and CH_PB8, CH_PB3) off In1 to get one plane.
- [ ] **Board v1.1, USB3 SuperSpeed (blocks USB3, D-032)**: board #1 never trains SuperSpeed (CH569 link counters:
  termination detect + LFPS + TX EQ on every attempt, LINK_RDY never; two USB3 cables, 4 PHY settings; pins, AC
  caps, crystal checked). Measured on `rp569-routed.kicad_pcb` (`board_metrics.py`): USB3_SSRX P/N 44.5/37.9 mm,
  skew 6.5 mm, 3 vias on N only, 4.7 % of N off the In1 plane; USB3_SSTX 15.4/10.1 mm, skew 5.3 mm (CH_SSTX 0.0 mm).
  To do: re-route USB3_SSRX and USB3_SSTX as coupled 90 Ω pairs (D-016 width/gap), intra-pair skew <= ~1 mm
  (5 Gb/s bit = 200 ps ~ 30 mm), no vias or the same vias on both legs, continuous GND reference under the whole
  pair, ESD (TPD2EUSB30) and SSTX AC caps placed in line so the pair stays coupled, short stubs. Check with
  `board_metrics.py` (skew, vias, plane coverage) before ordering; on the new board, console `usb3` must show
  `ready` > 0 and `lsusb -t` 5000M.
- [ ] **Board v1.1: SerDes test path (CH569 GXM/GXP, pins 65/66, not connected on board #1)**:
  - Background: CH569DS1 chapter 16 gives one differential pair, used in both directions, up to 1.2 Gb/s. V33GX
    (pin 64) is already on 3V3_CH.
  - Why a second CH56x: the framing is WCH's own (8b/10b, SATA-style ALIGN/CONT, CRC, sequence numbers; HydraUSB3
    `wch-ch56x-bsp` `drv/CH56x_serdes.*`), so no generic SerDes chip can be the peer.
  - Rates in that driver: 300 Mb/s to 1.2 Gb/s, plus experimental 180 / 240 Mb/s and up to 1.38 Gb/s.
  - Routing: GXM/GXP as a coupled 100 Ω pair (the impedance of Cat5e/6) to a 2-pin 0.1" header.
  - Parts on the pair: two 0402 series pads (0 Ω or AC caps, to be chosen) and two test pads.
  - Peer: a second assembled rp569 board (order at least 2 assembled). The CH565W/CH565M also have SerDes, but a
    second chip on the board is only worth it if it is needed anyway.
  - Optional: an RJ45 footprint left unfitted.
  - Power: SerDes takes ~60 mA with RX and TX on (CH569DS1 current table); add it to the AP2171W 1 A budget
    item below.
  - First check (no peer): `RB_SERD_DN_TST` (`R16_SERD_ANA_CFG1` bit 9) puts an internal voltage on GXM; measure it
    on the test pad.
  - Then two boards: short twisted pair, then Cat6, at 300 → 600 → 1200 Mb/s. Count CRC and sequence errors.
  - Not pursued: an FPGA receiver over LVDS (the WCH framing would have to be decoded) and RP2350 HSTX as a sender
    (send-only, its GPIO12-19 are the HSPI bus).
- [ ] **Board v1.1: RP2350 measures the CH569 1.2 V rail (1V2_CH)**:
  - Why: 1V2_CH was never measured on board #1. CH569DS1 requires V12USB at 1.15-1.3 V, and that rail is the one
    still unchecked in the USB3 diagnosis (D-032).
  - The ADC pins are all in use: GPIO46/47 = VBUS3 and 3V3_CH sense, GPIO40-45 = the EXT header.
  - Proposal: route 1V2_CH to one ADC, e.g. GPIO45/ADC5 taken from the EXT header, or through a solder jumper so the
    header pin stays usable. Add a ~1 kΩ series resistor and no divider: 1.2 V is below the 3.3 V ADC reference.
  - Keep the power.md back-power rules: the CH569-off case must not feed the rail through the ADC pin.
  - Firmware: report the rail in the RP2350 CLI next to 3V3_CH, with the matching python `@command`.
  - Board #1 meanwhile: measure 1V2_CH with a multimeter.
- [ ] USB2 pairs in the same pass (they work, but are out of spec): USB3_D skew 5.8 mm (0/5 vias), USBC_D 5.2 mm,
  RP_USB_D 8.9 mm (0/4 vias): coupled, matched, symmetric vias.
- [ ] HSPI lengths: 19.9–32.6 mm vs RCLK 20.6 mm; tune outliers only if the bus runs near its limit (target ~±10 mm).
- [ ] Silkscreen: ~68 labels over copper, connector/button labels (header pin labels on B.SilkS and RPDFU/BOOT done).
- [ ] Place the USB3 ESD (TPD2EUSB30) in line with each pair so the pairs can stay coupled (part of the v1.1 USB3 item above).
- [ ] Silkscreen cleanup.
- [ ] Consigned parts: CH569W and GSB343K33HR (LM66100 replaced by TPS2121, D-015).
- [ ] Check JLCPCB rotations in the placement preview; recheck stock before ordering.
- [ ] Confirm the 3.3 µH alternative against the RP2350 HW design guide (AOTA-B201610S3R3 has 119 in stock).
- [ ] Check AP2171W 1 A limit vs. CH569 + SD peak current (D-009).

## Firmware (D-024; `firmware/`, skill `firmware`)

- [ ] Hardware check of the python library (`python3 -m rp569 ports` / `versions`, `link_test.py`): port roles, CH569 UID read at 0x77FE4.
- [ ] CH569 USB3 (D-032): link training fails on board #1 (SS pair skew 6.5 / 5.3 mm, single-leg vias on SSRX_N). Board v1.1: re-route USB3_SSRX/USB3_SSTX as coupled, length-matched pairs. Meanwhile try a short cable / other port / USB3 hub. Then confirm the composite (and the MSC write fix) on USB3; decide whether the USB2 fallback needs a serial port (wch-ch56x-lib).
- [ ] CH569 UID: `chip_uid.c` reads `0xF7F9BF11` at every address of 0x77FE0–0x77FFC (stale ROM data register), so the USB serial is `11BFF9F711BFF9F7` on board #1 instead of the bootloader's UID 8A-82-48-26-3B-38-0D-E1. Same sequence/registers as HydraUSB3 `FLASH_ROMA_READ`; `R8_GLOB_ROM_CFG` reads 0x00 and a safe-mode write does not stick. Try: read before `SystemInit`/clock change, compare with WCH's EVT `GetUniqueID`/ISP library. Lead (2026-11-20):
  `0xf7f9bf11` is a `.word` in our vector table (`startup_CH56x.S`), so the read returns our image; and never mask
  interrupts with `__disable_irq()` (user mode: it traps, D-034), use `gintenr`.
- [ ] Streamed WRITE10 (one CMD25 through the ring with AUTOGAPSTOP, `MSC_WRITE_STREAM`): broken, disabled. On
  2026-11-19 it corrupted the FAT32 boot sector of card MSSD0 (single-sector writes: the n = 1 mode needs TRAN_MODE
  bit 6; multi-block failed with write I/O errors). Next attempt only with raw writes to a test region far from the
  filesystem (e.g. the last 64 MB of the card), never through a mounted volume.
- [ ] Card MSSD0 (DCFC-DA1E): user to repair (`sudo fsck.vfat`, backup boot sector) or reformat.
- [ ] CH569 drive speed on USB3: port `msc_fast.c` to the SuperSpeed endpoints (1024 B packets, ERDY) once a SuperSpeed link works; USB2 is at 5.0 / 17.6 MB/s.
- [ ] Card 573B-9367: FAT dirty flag from the early unclean power cuts, and FAT writes lost at 20:07:15 (2026-11-19, reflash under a stuck write): `sudo fsck.vfat -a /dev/sdb1` once (user, needs root).
- [ ] Add the ModemManager-ignore lines to `/etc/udev/rules.d/60-rp569.rules` (`docs/toolchain.md`, needs sudo).
- [ ] Install wch-ch56x-isp permanently (built in the scratchpad 2026-11-19: `git clone --recursive https://github.com/hydrausb3/wch-ch56x-isp`, `make`, copy to `~/.local/bin`).
- [ ] Variant 1 next: logic analyser on GPIO0-7 over the stream path; USB3 bulk (stage 3); RX_LEN0/1 and RB_HSPI_NUM_MIS meaning (D-031 open points).
- [ ] Variant 1 host reader: async multi-URB (python `libusb1`, needs install: user) so 44 MB/s is lossless; sync
  pyusb reads lose 1-3 packets/s at 35-40 MB/s (D-033). Optional: HSPI R_DONE by interrupt (shorter RP2350 gap),
  2 more ring slots (11.9 KB RAMX spare).
- [ ] CH569 test 1: HSPI CH569 -> RP2350 (CH569 HSPI TX, RP2350 PIO receiver on HD0-7 + HTCLK/HTREQ/HTVLD, HTRDY).
- [ ] CH569 low-power sleep / wake on the link UART (no current measurement on board #1: function only); optional
  fed watchdog in the variant 0 main loop (a hang would restart the CH569 by itself, D-034).
- [ ] RP2350: I2C link and breakout I2C commands; PWM dimming for the LEDs (GPIO34/35/38 are PWM9A/9B/11A).
