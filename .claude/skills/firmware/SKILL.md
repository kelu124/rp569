---
name: firmware
description: Use when building, changing or flashing the rp569 firmware (firmware/rp2350 RP2350B CLI, firmware/ch569 CH569 USB3 composite) — toolchains on this machine, build commands, flash/bring-up sequence, pin sources, known traps.
---

# rp569 firmware

Two independent builds; both work on this machine (2026-10-24). Neither has run on hardware yet.

| | RP2350B (`firmware/rp2350`) | CH569W (`firmware/ch569`) |
|---|---|---|
| Build | `firmware/rp2350/build.sh` | `firmware/ch569/build.sh` (`build.sh check` = static checks) |
| Toolchain | Pico SDK 2.2.0 `~/.pico-sdk/sdk/2.2.0` (has TinyUSB), `/usr/bin/arm-none-eabi-gcc` 10.3, cmake 3.25, ninja | xPack `riscv-none-elf-gcc` 15.2 in `~/.local/xPacks/@xpack-dev-tools/riscv-none-elf-gcc/15.2.0-1.1/.content/bin` (build.sh adds it to PATH) |
| Output | `build/rp569_rp2350.uf2` (~76 KB) | `build/rp569_ch569.bin` (~21 KB) / `.hex` |
| Flash | RPDFU (BOOTSEL) + RUN → copy uf2, or `picotool load -x` (`/usr/local/bin/picotool` 2.3); running fw: `dfu` command or `picotool load -f -x` | `ch boot` in the RP2350 CLI → `wch-ch56x-isp -v -f build/rp569_ch569.bin` or `wchisp flash` within ~10 s (neither tool installed yet: ask the user) |
| Pins | `boards/rp569.h` (`RP569_*`), from `hardware/gen/board.py` | `src/bridge_config.h` |

The shell env sets `PICO_SDK_PATH=/home/kelu/projets/pico-sdk/` (SDK 2.1.0); `build.sh` ignores it and uses 2.2
(override: `RP569_PICO_SDK=...`). `~/pico/pico-sdk` is 1.5 (no RP2350): never use it.

## RP2350 firmware

- `src/main.c`: CLI over stdio (USB CDC + UART0 GPIO32/33). Add a command in `run()` and a line in `help()`.
- CH569 control = specification.md D-004 with post-D-018 pins: PWR_EN GPIO29 (100k pull-down: CH569 off until
  driven), FLG# GPIO30, RST# GPIO28 open-drain (only works with RESET_EN set), boot strap GPIO8 = HSPI_D0 → PA5.
- While the CH569 is off, every line to it must be an input without pull-up (`ch_lines_release`), else it is
  back-powered. Normal power-up: GPIO8 pull-up only (jumper/button must still win).
- LEDs are common anode: pin low = on. ADC: GPIO40–47 = ADC0–7 (RP2350B); VBUS3 divider 147/47, 3V3_CH ×2.

## CH569 firmware

- Fork of `~/projets/CH569_Composite` (user's repo, github.com/kelu124/CH569_Composite). Keep changes listed in
  `firmware/ch569/README.md` § Changes; port upstream fixes by diffing `src/`.
- UART1 PA7/PA8 = RP2350 link (CDC #1 bridge + echo, `Link_Echo_Deal` in the main loop). UART0 PB5/PB6 = debug,
  `-DDEBUG=0`. Never set `RB_PIN_UART0` (moves UART0 to PA5/PA6 = boot strap / HSPI).
- The SDK `PRINT` macro was patched (CH56xSFR.h) to wait on the UART chosen by `DEBUG`.
- CDC ports exist only on a USB3 link (vendor USB2 stack is MSC-only).
- IRQ handlers: plain `__attribute__((interrupt))` (upstream GCC ignores MounRiver's "WCH-Interrupt-fast").

## Testing

- `python3 firmware/tools/link_test.py /dev/ttyACM0 [-n 20]` (pyserial): `ch send ping i` → expects `CH> ping i`.
- Without hardware: both `build.sh` must finish without compiler warnings (the CH569 link prints
  `_close/_fstat/... not implemented` notes: expected). `firmware/ch569/build.sh check` must print "all good".
