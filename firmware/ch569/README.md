# CH569W firmware

USB 3.0 SuperSpeed composite device (`1a86:fe2c`), forked from
[CH569_Composite](https://github.com/kelu124/CH569_Composite) (itself from the WCH CH569 EVT examples):

| Function | Host side | Backed by |
|---|---|---|
| CDC-ACM #1 | serial port (`ttyACMx`, lower number) | **UART1** to the RP2350 (PA7 RX / PA8 TX, 115200 8N1): what the RP2350 sends shows up here, what the host types goes to the RP2350 |
| CDC-ACM #2 (read-only) | serial port | the firmware's debug log (also on UART0, PB6 TX, CH UART header J8) |
| Mass storage | removable drive | the microSD card (raw sectors), mounted at power-up |

**Link echo**: every byte received from the RP2350 is sent back to it (`LINK_ECHO` in `src/bridge_config.h`), from
the main loop through a 256-byte ring, with or without a USB host. The RP2350 CLI shows the echo as `CH> ...`.

**RGB LED** (active low): R = SD card ready, G = USB3 link up, B toggles on every line from the RP2350.

## Changes from CH569_Composite

- CDC bridge on UART1 (RP2350 link) instead of UART2 (FPGA link on PA2/PA3: HSPI pins on rp569), plus the echo;
  the CDC RX ring drops bytes when full instead of wrapping its count.
- Debug log on UART0 (PB5/PB6, `-DDEBUG=0`), UART0 pinned to PB5/PB6 (`RB_PIN_UART0` cleared: its alternate pins
  PA5/PA6 are HSPI_D0/boot). The SDK `PRINT` macro now waits on the UART that `DEBUG` selects (it always waited on
  UART1).
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
