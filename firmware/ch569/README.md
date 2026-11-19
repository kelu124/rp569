# CH569W firmware

USB device (`1a86:fe2c`, the same at SuperSpeed and on the USB2 high-speed fallback) with two modes, forked from
[CH569_Composite](https://github.com/kelu124/CH569_Composite) (itself from the WCH CH569 EVT examples):

| Function | Host side | Backed by |
|---|---|---|
| CDC-ACM #1 | serial port (`ttyACMx`, lower number) | **UART1** to the RP2350 (PA7 RX / PA8 TX, 115200 8N1, fixed: the host's line coding is ignored, `LINK_FOLLOW_HOST_BAUD`): what the RP2350 sends shows up here, what the host types goes to the RP2350 |
| CDC-ACM #2 (console) | serial port | the firmware's debug log (also on UART0, PB6 TX, CH UART header J8) and a small console: `help`, `version`, `status` (prompt `ch569> `; also on UART0 RX) |
| Mass storage | removable drive | the microSD card (raw sectors), mounted at power-up |

**Two modes** (`src/mode.c`, specification D-028), one USB personality at a time:

| Mode | Host sees | Entered by |
|---|---|---|
| **drive** (power-up) | the SD card as a USB drive, nothing else (full speed for the card) | power-up with a card; console `mount`; RP2350 `ch mount` (answer `\x01SDON`) |
| **serial** | two serial ports: link CDC (if0) + console CDC (if2), no drive | host eject (desktop eject / `eject`: SCSI START STOP UNIT LoEj=1); RP2350 `ch eject` (answer `\x01SDOFF`); no card at power-up |

A switch happens in place, no chip reset (a software reset leaves the CH569 dead, CH569DS1 §3.2): USB off, card
released (flushed, CMD0, clock stopped; it stays powered, no switch on its supply) or mounted, 300 ms off the bus,
USB restarted with the other mode's descriptors. `udisksctl power-off` also disables the host port, so the
serial ports do not appear after it: use the desktop eject / `udisksctl unmount` + `python -m rp569 eject`.

**Mode flags for host software**: the USB product string names the mode ("rp569 CH569 drive" / "rp569 CH569
serial", readable from `lsusb`, udev or sysfs even in drive mode; python `ports.ch569_usb_mode()`), and the CH569
sends `\x01MDRV` / `\x01MSER` on the link at boot and after every switch: the RP2350 prints an unsolicited
`CH569 mode: drive|serial` line on its own serial port and shows the mode in `ch status` (python
`RP2350.ch569_mode_events()`, `ch_status()['mode']`).

**Reset and card detect** (tested on board #1, 2026-11-19, D-030):
- RST# (PB15) resets the CH569 only with RESET_EN set in its config (factory: off, PB15 is a GPIO; set it with the
  patched wch-ch56x-isp, `docs/toolchain.md`). A pin reset reloads the firmware like a power-up (USB re-enumerates,
  the mode and card flags are announced again); RP2350 `ch reset` releases the card first. A *software* reset
  (`RB_SOFTWARE_RESET`) does not: the chip stays dead (CH569DS1 §3.2), so the mode switch is done in place.
- SD_DET (PA20, 100k pull-up): low = card in. Polled every main loop, 50 ms debounce. In drive mode a removal releases
  the card (host: medium not present, red LED off) and an insertion identifies and mounts the new card (host:
  medium changed, red LED on). `\x01SDIN`/`\x01SDOUT` on the link at boot and on every change: the RP2350 prints
  `CH569 card: in|out` and shows it in `ch status`; console `sd` ends with `det in|out`.

**USB3 diagnostics**: console `usb3` (python `usb3_diag()`) prints the SuperSpeed link interrupt counts since boot:
`rx_detect` / `present` / `txeq` (termination and LFPS stages), `ready` (link up), `disabled`, `timeouts` (USB2
fallbacks). Board #1: rx_detect, present and txeq on every attempt, ready never (D-032, SS pair routing).
`./build.sh EXTRA_CFLAGS=-DUSB3_LINK_CFG=0x1C2` overrides the PHY config during training (experiments).

**Link control lines**: a line starting with byte `0x01` (`LINK_CTRL_CHAR`) is for the CH569 itself, neither echoed
nor forwarded to the host. `\x01SDREL\n` from the RP2350 releases the SD card: the CH569 waits for any MSC transfer to
finish, waits until the card is idle (DAT0 high, 500 ms max), reports "medium not present" to the host, then answers
`\x01SDOFF\n`. The RP2350 does this before every CH569 power-off (`ch off/cycle/boot`, 2 s timeout) and on `ch eject`.
The card stays released until the CH569 restarts. The host still has to unmount for a clean FAT volume:
`python -m rp569 eject` unmounts first, then releases.

**Link echo**: every byte received from the RP2350 is sent back to it (`LINK_ECHO` in `src/bridge_config.h`), from
the main loop through a 256-byte ring, with or without a USB host. The RP2350 CLI shows the echo as `CH> ...`.

**USB identity** (both speeds): `1a86:fe2c`, product "rp569 CH569", serial number = the chip unique ID, meant to be
the 8 factory bytes at flash ROM 0x77FE4 (CH569DS1 §2.2.3, read as in HydraUSB3: `src/chip_uid.c`). **Known bug**
(board #1, 2026-11-19): the ROM read returns a stale `0xF7F9BF11` for every address, so the serial is
`11BFF9F711BFF9F7`, not the UID the bootloader reports (8A82…E1); see `.claude/todo.md`. Interfaces: 0 = drive
(EP 0x82/0x03), 1/2 = link CDC (0x81 notify, 0x84/0x04), 3/4 = console CDC (0x85, 0x86/0x06); the python library
tells the two ports apart by interface number.

**Version**: `version.yaml` (major/minor by the user, patch bumped by the pre-commit hook) → `build/generated/fw_version.h`
with the `git describe` hash; `version` on the console prints
`rp569 CH569W firmware x.y.z git <hash> built <date> uid <16 hex>`. Console commands are one table in
`src/console.c` (`COMMANDS-BEGIN` / `COMMANDS-END`), mirrored by `python/rp569/ch569.py` (pre-commit check).

**RGB LED** (active low): R = SD card ready, G = USB3 link up, B toggles on every line from the RP2350.

## Changes from CH569_Composite

- CDC bridge on UART1 (RP2350 link) instead of UART2 (FPGA link on PA2/PA3: HSPI pins on rp569), plus the echo;
  the CDC RX ring drops bytes when full instead of wrapping its count.
- Debug log on UART0 (PB5/PB6, `-DDEBUG=0`), UART0 pinned to PB5/PB6 (`RB_PIN_UART0` cleared: its alternate pins
  PA5/PA6 are HSPI_D0/boot). The SDK `PRINT` macro now waits on the UART that `DEBUG` selects (it always waited on
  UART1), and made a single statement (`do { } while (0)`: it was two statements, wrong in an unbraced loop).
- Console on the debug port (EP6 OUT was discarded) and UART0 RX; USB product/serial strings for rp569.
- No SD ownership line (`SD_OWNERSHIP_PIN 0`, the card is wired straight to the CH569); `CLAIM_ON_BOOT 1`.
- Status LEDs on PB0/PB1/PB2 instead of the eval board's PB22–24 (CH EXT header pins on rp569).
- USB2 fallback rewritten as the same composite (was the vendor's drive-only stack): `CH56x_usb20.c` descriptors,
  CDC class requests, SD-ownership vendor request, CLEAR/SET_FEATURE on every endpoint (the vendor code wrote the
  TX register for 0x02 and ignored 0x03); `cdc_uart.c` / `debug_cdc.c` pumps drive either register set.
  `tools/check_descriptors.py` checks both layouts.
- MSC write fix: EP3 re-armed after the last data packet of a WRITE10 (it stayed NAKed, so every write ended in a
  30 s host timeout + USB reset).
- stdout unbuffered (`setvbuf`): the `ch569> ` prompt has no newline and stayed in the stdio buffer.
- Link UART1 rate no longer follows the host's SET_LINE_CODING (cdc_acm/ModemManager set 9600 on open).
- `tools/check_mem.py` in the build: RAMX use vs the RAMX the flasher leaves for the image size.
- Drive speed: multi-block SD reads/writes (`SDCardReadMulSec` / `SDCardWriteMulSec` in `SD.c`, HydraUSB3 BSP
  pattern; multi-block writes need TRAN_MODE = DMA_DIR only and `R32_EMMC_RESPONSE3 = 0` at each block gap), CMD6
  high-speed switch (`SD_SwitchHighSpeed`, own zeroed DMA buffer), SuperSpeed MSC packets of 1024 B (512 B was a short
  packet). A failed write now queues the CSW after stalling bulk OUT (BOT 6.7.3) instead of leaving the host to time out.

## Build and flash

```sh
./build.sh                     # xPack riscv-none-elf-gcc (found under ~/.local/xPacks) -> build/rp569_ch569.bin
./build.sh check               # include-case + USB descriptor static checks
```

Flash: unmount the drive, `ch boot` in the RP2350 CLI (or the CH BOOT jumper / BOOT button at power-up), then within
~10 s `wch-ch56x-isp -vr -f=build/rp569_ch569.bin` (it resets the chip into the new firmware). wchisp 0.3.0 cannot
flash bootloader 2.70; a new chip first needs `-d=off` and `-b=off` (`docs/toolchain.md`).

## Limits

- Serial mode has no drive and drive mode no serial port: the RP2350 link UART and its echo work in both.
- **Drive speed** (drive mode, USB2, board #1, 32 MB of numbered sectors verified): ~5.0 MB/s write, ~17.6 MB/s read
  with `msc_fast.c` (D-029: USB interrupt off for one READ10/WRITE10, CMD18 into an 80-sector ring with AUTOGAPSTOP,
  80-sector CMD25 chunks double-buffered against the card's programming time; ported from phantomdrive). The USB3
  path still uses the per-packet loop below (not testable yet). Before msc_fast: ~3.8 MB/s write, ~8.4 MB/s read
  (was 0.25 / 1.06 MB/s).
  One CMD18 / CMD25 per 64 sectors (`MSC_CHUNK_SECTORS`, 32 KB in `UDisk_In_Buf` / `UDisk_Out_Buf`), card in
  high-speed mode at 48 MHz (CMD6; 24 MHz if the card refuses). The limit now is one USB packet per main-loop pass
  with SD and USB never overlapping; phantomdrive's pipelined, USB-IRQ-masked loop would go further at the cost of
  pausing the serial ports during copies.
- USB2 fallback is high-speed only (descriptors declare 512-byte bulk packets; no full-speed variant).
- The serial ports pause during large drive transfers (USB interrupt masked by the MSC engine); UART1 RX is
  interrupt-driven and buffered, so nothing from the RP2350 is lost.
- On Linux, stop ModemManager probing the ports (it sends `AT` lines to the RP2350): `ID_MM_DEVICE_IGNORE` in
  `/etc/udev/rules.d/60-rp569.rules`, see `docs/toolchain.md`.
