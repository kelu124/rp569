# CH569W firmware

USB composite device (`1a86:fe2c`), the same at SuperSpeed and on the USB2 high-speed fallback, forked from
[CH569_Composite](https://github.com/kelu124/CH569_Composite) (itself from the WCH CH569 EVT examples):

| Function | Host side | Backed by |
|---|---|---|
| CDC-ACM #1 | serial port (`ttyACMx`, lower number) | **UART1** to the RP2350 (PA7 RX / PA8 TX, 115200 8N1, fixed: the host's line coding is ignored, `LINK_FOLLOW_HOST_BAUD`): what the RP2350 sends shows up here, what the host types goes to the RP2350 |
| CDC-ACM #2 (console) | serial port | the firmware's debug log (also on UART0, PB6 TX, CH UART header J8) and a small console: `help`, `version`, `status` (prompt `ch569> `; also on UART0 RX) |
| Mass storage | removable drive | the microSD card (raw sectors), mounted at power-up |

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

## Build and flash

```sh
./build.sh                     # xPack riscv-none-elf-gcc (found under ~/.local/xPacks) -> build/rp569_ch569.bin
./build.sh check               # include-case + USB descriptor static checks
```

Flash: unmount the drive, `ch boot` in the RP2350 CLI (or the CH BOOT jumper / BOOT button at power-up), then within
~10 s `wch-ch56x-isp -vr -f=build/rp569_ch569.bin` (it resets the chip into the new firmware). wchisp 0.3.0 cannot
flash bootloader 2.70; a new chip first needs `-d=off` and `-b=off` (`docs/toolchain.md`).

## Limits

- **Drive speed** (USB2, board #1): ~0.25 MB/s write, ~1 MB/s read: one 512-byte sector per USB packet and
  single-sector SD commands (`MSC_CHUNK_SECTORS 1` in `SW_UDISK.c`).
- USB2 fallback is high-speed only (descriptors declare 512-byte bulk packets; no full-speed variant).
- The serial ports pause during large drive transfers (USB interrupt masked by the MSC engine); UART1 RX is
  interrupt-driven and buffered, so nothing from the RP2350 is lost.
- On Linux, stop ModemManager probing the ports (it sends `AT` lines to the RP2350): `ID_MM_DEVICE_IGNORE` in
  `/etc/udev/rules.d/60-rp569.rules`, see `docs/toolchain.md`.
