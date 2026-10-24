# CH569W firmware

USB 3.0 SuperSpeed composite device (`1a86:fe2c`), forked from
[CH569_Composite](https://github.com/kelu124/CH569_Composite) (itself from the WCH CH569 EVT examples):

| Function | Host side | Backed by |
|---|---|---|
| CDC-ACM #1 | serial port (`ttyACMx`, lower number) | **UART1** to the RP2350 (PA7 RX / PA8 TX, 115200 8N1): what the RP2350 sends shows up here, what the host types goes to the RP2350 |
| CDC-ACM #2 (console) | serial port | the firmware's debug log (also on UART0, PB6 TX, CH UART header J8) and a small console: `help`, `version`, `status` (prompt `ch569> `; also on UART0 RX) |
| Mass storage | removable drive | the microSD card (raw sectors), mounted at power-up |

**Link echo**: every byte received from the RP2350 is sent back to it (`LINK_ECHO` in `src/bridge_config.h`), from
the main loop through a 256-byte ring, with or without a USB host. The RP2350 CLI shows the echo as `CH> ...`.

**USB identity**: `1a86:fe2c`, product "rp569 CH569", serial number = the chip unique ID (8 factory bytes at flash
ROM 0x77FE4, CH569DS1 §2.2.3, read as in HydraUSB3: `src/chip_uid.c`). Interfaces: 0 = drive, 1/2 = link CDC,
3/4 = console CDC; the python library tells the two ports apart by interface number.

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

## Build and flash

```sh
./build.sh                     # xPack riscv-none-elf-gcc (found under ~/.local/xPacks) -> build/rp569_ch569.bin
./build.sh check               # include-case + USB descriptor static checks
```

Flash: `ch boot` in the RP2350 CLI (or the CH BOOT jumper / BOOT button at power-up), then within ~10 s
`wch-ch56x-isp -v -f build/rp569_ch569.bin` or `wchisp flash build/rp569_ch569.bin`, then `ch cycle`.

## Limits (inherited)

- **USB2 fallback is drive-only**: on a USB2 port/cable there is no serial port (the vendor USB2 stack has no CDC).
- The serial ports pause during large drive transfers (USB interrupt masked by the MSC engine); UART1 RX is
  interrupt-driven and buffered, so nothing from the RP2350 is lost.
- On Linux, stop ModemManager probing the ports: see the udev rule in the CH569_Composite README.
