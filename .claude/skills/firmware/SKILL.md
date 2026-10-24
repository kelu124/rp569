---
name: firmware
description: Use when building, changing or flashing the rp569 firmware (firmware/rp2350 RP2350B CLI, firmware/ch569 CH569 USB3 composite) or the python host library (python/rp569) — toolchains, build commands, flash/bring-up, command tables and their python mirror, versions and the pre-commit hook, known traps.
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

## Commands, python library, versions (D-025)

- Commands = one table per firmware between `COMMANDS-BEGIN` / `COMMANDS-END` (`firmware/rp2350/src/main.c`,
  `firmware/ch569/src/console.c`). Adding/renaming/removing one means: edit the table, add/rename the
  `@command("<name>")` method in `python/rp569/rp2350.py` / `ch569.py` (+ its reply regex), a unit test in
  `python/tests/`, the README help block. `python3 tools/check_commands.py` must say "in sync".
- Reply formats are parsed by regex in the python methods: changing a `printf` format = changing the method.
  `version` lines: `rp569 <CHIP> firmware x.y.z git <hash> built <date> uid <hex>` (`FirmwareVersion.parse`).
- USB identity (python discovery relies on it): RP2350 `2e8a:0009` product "rp569 RP2350B" (CMake
  `USBD_PRODUCT`); CH569 `1a86:fe2c` product "rp569 CH569" (`StringProduct` in `CH56x_usb30.c`), serial = chip UID
  (`USB30_SetSerial`), link CDC = interface 1, console = interface 3.
- Versions: `firmware/*/version.yaml` + `python/rp569/_version.py`. Hook `.githooks/pre-commit` (enabled by
  `tools/install_hooks.sh`, `core.hooksPath`) runs the command check and `tools/bump_versions.py` (patch +1 per
  touched component, staged automatically). **Never edit major/minor**: user only. `bump_versions.py --show`.
- Python tests: `cd python && python3 -m unittest discover -s tests -v` (no hardware, fake firmware). pytest is not
  installed; pyserial 3.5 is.

## Testing

- `PYTHONPATH=python python3 firmware/tools/link_test.py [-n 20]`: versions of both chips, then `ch send ping i`
  → expects the echo. `PYTHONPATH=python python3 -m rp569 ports` lists the port roles.
- Without hardware: both `build.sh` must finish without compiler warnings (the CH569 link prints
  `_close/_fstat/... not implemented` notes: expected). `firmware/ch569/build.sh check` must print "all good".
