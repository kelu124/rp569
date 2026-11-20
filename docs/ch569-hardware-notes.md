# CH569W hardware notes

Taken from `datasheet/CH569DS1.pdf` and from phantomdrive's KiCad design
(`project/phantomdrive/ee/ovrdrive/`, netlist exported with `kicad-cli sch export netlist`).

## Power (phantomdrive pattern)

| Rail | Source in phantomdrive | CH569 pins |
|---|---|---|
| +3V3 | TLV62569DBV buck from VBUS, 2.2 µH | VDDIO 16/42/61, V33USB 3, V33LDO 62, V33GX 64, SD card VDD |
| +1V2 | TLV62569DBV buck from VBUS, 2.2 µH | V12CORE 9 and 63, V12USB 6 |

- The datasheet describes pin 63 as the *output* of the internal 1.2 V LDO (3.3 µF). phantomdrive
  overrides it with an external 1.2 V buck, probably to keep the USB3 PHY current off the internal LDO.
  We follow phantomdrive (proven design). Check the datasheet's electrical section for the CH569's current draw.
- Decoupling: 100 nF per supply pin, plus 22 µF bulk per rail (phantomdrive uses 0402 22 µF parts).

## Clock

- 30 MHz crystal is required (datasheet §3, HSE feeds the USB PHY). phantomdrive: 3225 4-pin, 15 pF load caps.

## Boot, reset, flashing

- **Boot pin**: PA5 (pin 15) or PA13, chosen by a config bit. **Chips as delivered (board #1) select PA13**
  (`alt boot pin: PA13`, NV word `0xCFFFF265`); rp569 uses PA5 (held low by the RP2350, GPIO8): set it once with
  `wch-ch56x-isp -b=off` (it invalidates the image). With PA13, `ch boot` only works while the code flash is empty.
- USB bootloader enumerates as `4348:55E0` and times out after ~10 s.
- **RST# on PB15 is disabled by default** (config bit RESET_EN = 0, PB15 is a GPIO). Set it with the patched
  wch-ch56x-isp (`firmware/tools/wch-ch56x-isp-resetpin.patch`, `-e=on`); board #1: NV `0xCFFFF255`.
- **DEBUG_EN = 1 on delivered chips**: the USB bootloader then refuses to program (`Fail to program chunk @ 0 error:
  e0 00`); clear it with `wch-ch56x-isp -d=off`. Details and the full flashing procedure: `docs/toolchain.md`.

## USB3

- phantomdrive: USB3 Type-A plug (Würth 692112030100), TPD2EUSB30 ESD on D±, SSRX± and SSTX±,
  100 nF series caps on SSTX between the chip and the connector.

## SD card

- 4-bit on the eMMC controller: CLK PB14, CMD PB16, D0–D3 PB17–PB20. 47k pull-ups on CMD/D0–D3
  (and CLK via the resistor array). Card detect on PA15. Hirose DM3AT micro-SD socket.

## Undocumented behaviour found on board #1 (2026-11-19)

Measured on the rp569 board, not in CH569DS1 (V1.x) or contradicting it. Each point: what we saw, what the
firmware does about it, where. Decisions: `specification.md` D-026..D-032.

### Bootloader, flashing, configuration

- **Bootloader 2.70 replies `00` to the ISP key command.** wchisp 0.3.0 expects the checksum of the UID-derived key
  and stops (`isp_key checksum failed`); with that check patched out it still fails verify. wch-ch56x-isp handles
  bootloader >= 2.7 (the key stays UID-derived): use it.
- **wch-ch56x-isp rewrites USER_MEM from the image size at every flash**: < 32 KB -> ROM 32 KB + RAMX 96 KB, < 64 KB ->
  64 + 64, < 96 KB -> 96 + 32. A bigger image silently shrinks RAMX under the linker's 96 KB: `tools/check_mem.py` fails
  the build first. Its `devices.h` defines `SZ_96K` as 64 KB, so it refuses images >= 64 KB.
- **Safe-access writes (`R8_SAFE_ACCESS_SIG` 0x57/0xA8) are silently ignored if an interrupt lands in between**
  (TMR2 ticks every 10 us here): mask interrupts (`__disable_irq()`) around the signature and the protected write.
  `R8_GLOB_ROM_CFG` read 0x00 and did not take a safe-mode write even so (open).
- **A software reset (`RB_SOFTWARE_RESET`) leaves the chip dead**: the program is copied into SRAM only at power-on
  (CH569DS1 §3.2 hints at it). A pin reset (RST# with RESET_EN) does reload and run the firmware. The drive/serial
  mode switch therefore re-initialises USB in place (`mode.c`), it never resets.
- **Chip UID**: reading flash ROM 0x77FE4 with the HydraUSB3 sequence returns a stale `0xF7F9BF11` for every address
  (the config word the bootloader read last); the real UID (bootloader: 8A-82-48-26-3B-38-0D-E1) is not obtained yet
  (open; the USB serial number is therefore not unique per board).

### USB2 (USBHS) device controller

- **`RB_USB_INT_BUSY` auto-NAKs only while the transfer flag is set**: on an OUT endpoint, set the endpoint to NAK
  *before* clearing `RB_USB_IF_TRANSFER`, or the host's next packet lands on the same buffer (`msc_fast.c`: 28 of
  512 sectors were overwritten). Check `RB_USB_ST_TOGOK` to drop a resent packet.
- **Never read-modify-write `R8_UEPn_TX_CTRL` while an IN can complete**: a completion between the read and the
  write has its auto-toggle (`RB_UEP_T_AUTOTOG`) undone, and the host drops the next packet as a retransmission
  (variant 1: one 512-byte chunk lost every few hundred while the bulk loop rewrote TX_CTRL on every pass). Write it
  only while the endpoint NAKs or is frozen by the flag.
- **`RB_USB_INT_BUSY` halves bulk IN throughput**: the host's IN right after each packet gets a NAK and backs off
  (22.7-22.8 MB/s, single or double buffer). Off, with EP2 double-buffered (table 12-4, BUF_MOD: DATA0 from
  `UEP2_TX_DMA`, DATA1 from `UEP2_RX_DMA`): 44.7 MB/s. Without it the controller moves straight on to the other
  buffer after a packet, so with only one armed it would send a stale buffer: variant 1 turns INT_BUSY on whenever
  fewer than two are armed. Two completions can merge into one transfer flag: count them from the data toggle.
- Vendor MSC example bugs (fixed in `SW_UDISK.c` / `CH56x_usb20.c`): EP3 not re-armed after the last WRITE10 data
  packet (every write ended in a 30 s host timeout); no CSW after a bulk-OUT stall; CLEAR_FEATURE wrote the TX
  register for EP 0x02 and ignored 0x03.
- The vendor USB2 stack is drive-only and enumerates as "WCH USB2.0 DEVICE" without a serial number: rp569 rewrote it
  (same identity and layouts as USB3).
- Linux cdc_acm / ModemManager set 9600 baud on open: a CDC port bridged to a fixed-rate UART must ignore the host's
  line coding (`LINK_FOLLOW_HOST_BAUD 0`).

### USB3 (USBSS)

- The closed library trains with `LINK_CFG = 0x140` (RX EQ + de-emphasis, no TX swing), as HydraUSB3's open
  re-implementation shows; the example's `LINK_CFG` write with TX_SWING is only on a U-state exit.
- **`USB30D_init(DISABLE)` hangs when called on the USB2 fallback** (SuperSpeed side already off): only stop the stack
  in use.
- Link interrupt sequence on a working PHY path: TERM_PRESENT -> RX_DET -> TXEQ -> RDY. Board #1 gets TERM_PRESENT,
  RX_DET and TXEQ on every attempt, then LINK_DISABLE, never RDY, with two different USB3 cables and four LINK_CFG
  values: low-frequency signalling (termination, LFPS) works, 5 Gb/s training does not -> SS pair routing (D-032).
  The console `usb3` command prints these counters.

### SD / eMMC controller

- Multi-block write (CMD25): `R32_EMMC_TRAN_MODE` must be `RB_EMMC_DMA_DIR` only, and `R32_EMMC_RESPONSE3 = 0` written at
  each block gap; the vendor single-block mode (`DMA_DIR | (1 << 6)`) stops after the first block. Single-block reads
  use CMD17 (a 1-block CMD18 never raises TRANDONE). Guards must scale with the block count; send CMD12 after a failed
  data phase or the card stays in receive-data state.
- The vendor code clocks the card at 48 MHz without the CMD6 switch to high speed (out of spec): rp569 does CMD6 and
  falls back to 24 MHz. **CMD6's 64-byte status must go to its own zeroed buffer**: reusing the SCR buffer returned
  stale or 16-byte-shifted data and the switch never happened.
- `EMMC_IRQHandler` (latches errors) clears *every* flag, BKGAP included: keep `EMMC_IRQn` off during a gap-stop stream.
- ACMD23 (pre-erase count) made card MSSD0 slower (6.7 vs 7.1 MB/s): off by default.
- microSD card detect (PA20, Hirose DM3AT switch, 100k pull-up): low = card in. The card supply is 3V3_CH (no switch).

### HSPI (8-bit, CH569 as the "down" receiver, RP2350 PIO as the sender)

- **HRVLD must rise while HRCLK is low.** Raised on a rising edge, the CH569 samples a stale byte first and every
  packet shifts by one byte.
- **Header layout** (sent LSB first): TLL2B bits [1:0], TSQN [5:2], USDF [31:6] (the datasheet figure reads either way).
- **CRC = CRC-16/USB**: polynomial 0x8005 reflected, init 0xFFFF, final XOR 0xFFFF, over header + payload, low byte
  first (found by a 32-variant sweep). Packets with a bad CRC are still written to the RX buffer (RB_HSPI_CRC_ERR set).
- `R16_HSPI_RX_LEN0/1` read 0 or 1, not the payload length (fixed-length packets are used instead).
- The expected RX sequence number (`R8_HSPI_RX_SC`) is not advanced the way the datasheet suggests and
  `RB_HSPI_NUM_MIS` stays set on most packets whatever RX_SC is set to (unused, counted only). `R32_HSPI_UDF1`
  holds the raw 32-bit header of the latest packet received (TLL2B/TSQN bits included), `R32_HSPI_UDF0` reads 0:
  there is no per-register packet number to tell which RX register a packet completed on.
- **The RX toggle (`RB_HSPI_RX_TOG`) moves when the next packet starts**: read after R_DONE it names the register the
  next packet completes on only while the bus is still idle. With back-to-back packets (2 idle clocks) the firmware
  went one register out of phase and handed the slot still being written to USB (old and new words interleaved,
  ~3 jumps per packet). The RP2350 leaves 12 us between packets, more than the CH569 main-loop pass (5-7 us at 120 MHz).
- HRCLK kept running between packets (like a CH569 transmitter's HTCLK) works; byte clock 25-75 MHz with 0 CRC errors
  (74.7 MB/s raw at 75 MHz). Stream to the host: 44.7 MB/s (USB2 ceiling, D-033). The first packet after a CH569
  power-up fails its CRC (dropped).
- SysTick (`SysTick->CNT`, FREQ_SYS/8) counts **down** once started with `CTLR = 1`.

### Board level

- The RP2350 must keep every line to the CH569 tri-stated while the CH569 is off (back-powering); 3V3_CH reads
  ~0.7-1.4 V from leakage then. At CH569 power-up the link UART sees one 0x00 glitch byte.
- Pairs measured on the routed board (`board_metrics.py`): USB3_SSRX skew 6.5 mm with 3 vias on N only, USB3_SSTX
  5.3 mm, USB3_D 5.8 mm, RP_USB_D 8.9 mm. USB2 works anyway; USB3 does not (D-032).
