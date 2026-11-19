---
name: firmware
description: Use when building, changing or flashing the rp569 firmware (firmware/rp2350 RP2350B CLI, firmware/ch569 CH569 USB3 composite) or the python host library (python/rp569) — toolchains, build commands, flash/bring-up, command tables and their python mirror, versions and the pre-commit hook, known traps.
---

# rp569 firmware

Two independent builds; both work on this machine. RP2350 firmware runs on the first board (2026-11-19: CDC, `version`,
`adc` 5.02 V / 3.16 V, LEDs, `ch boot` → CH569 bootloader `4348:55e0` appears). CH569 flashed (DEBUG_EN off, boot pin PA5); it only links at USB2 (no SuperSpeed yet), where it is now the same
composite as on USB3: drive (FAT32 card OK, ~3.8 MB/s write, ~8.4 MB/s read, 48 MHz high speed) + link CDC (if1) + console CDC (if3).
Link echo 20/20, CDC bridge both ways.
`ch pins` probes the CH569 → RP2350 line. The first byte after a CH569 power-up is a 0x00 glitch: `link_poll` drops non-printables.
Machine setup, udev rules and tool updates: `docs/toolchain.md`.

| | RP2350B (`firmware/rp2350`) | CH569W (`firmware/ch569`) |
|---|---|---|
| Build | `firmware/rp2350/build.sh` | `firmware/ch569/build.sh` (`build.sh check` = static checks) |
| Toolchain | Pico SDK 2.2.0 `~/.pico-sdk/sdk/2.2.0` (has TinyUSB), `/usr/bin/arm-none-eabi-gcc` 10.3, cmake 3.25, ninja | xPack `riscv-none-elf-gcc` 15.2 in `~/.local/xPacks/@xpack-dev-tools/riscv-none-elf-gcc/15.2.0-1.1/.content/bin` (build.sh adds it to PATH) |
| Output | `build/rp569_rp2350.uf2` (~76 KB) | `build/rp569_ch569.bin` (~21 KB) / `.hex` |
| Flash | RPDFU (BOOTSEL) + RUN → copy uf2, or `picotool load -x` (`/usr/local/bin/picotool` 2.3); running fw: `dfu` command or `picotool load -f -x` | `ch boot` in the RP2350 CLI → flasher within ~10 s. `wch-ch56x-isp -vr -f=build/rp569_ch569.bin` (wchisp 0.3.0 cannot flash bootloader 2.70). New chip: first `-d=off` (DEBUG_EN blocks programming, `e0 00`) and `-b=off` (download pin PA13 → PA5), one `ch boot` each. `docs/toolchain.md` |
| Pins | `boards/rp569.h` (`RP569_*`), from `hardware/gen/board.py` | `src/bridge_config.h` |

The shell env sets `PICO_SDK_PATH=/home/kelu/projets/pico-sdk/` (SDK 2.1.0); `build.sh` ignores it and uses 2.2
(override: `RP569_PICO_SDK=...`). `~/pico/pico-sdk` is 1.5 (no RP2350): never use it.

## Bring-up traps (first board, 2026-11-19)

- picotool needs `/etc/udev/rules.d/60-rp569.rules` (in place now), else "unable to connect". Without it, copying the
  uf2 to `/media/$USER/RP2350/` works.
- Quick CLI driver: pyserial on `/dev/ttyACM0`, send `cmd\r\n`, read until `rp569> `.
- System Rust is apt 1.75: `cargo install` needs `--locked` (otherwise `feature edition2024 is required`).

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
- **Modes** (D-028, `mode.c`): drive (MSC only) / serial (link CDC if0 + console CDC if2), switched in place
  (USB off, card release/mount, 300 ms, `usb_start()`); never a software reset (dead chip: code is only loaded
  into SRAM at power-on), never `USB30D_init(DISABLE)` on the USB2 fallback (hangs). Stop USB before releasing the
  card: the host sends commands right after an eject. Triggers: SCSI eject, link `\x01SDREL`/`\x01SDMNT`, console
  `mount`. Desktop eject over D-Bus: `gdbus call --system --dest org.freedesktop.UDisks2 --object-path
  /org/freedesktop/UDisks2/drives/EMMC_... --method org.freedesktop.UDisks2.Drive.Eject "{}"` (`eject` needs root,
  `udisksctl power-off` disables the host port). Test script pattern: scratchpad `modes.sh`.
- Drive-mode fast path (`msc_fast.c`, D-029, USB2 only): USB IRQ off, EP0 NAKed, polling. Trap: on an OUT packet
  NAK the endpoint **before** clearing `RB_USB_IF_TRANSFER` (the controller only auto-NAKs while the flag is set:
  clearing first let the next packet overwrite the same slot, 28/512 sectors wrong); check `RB_USB_ST_TOGOK`. Keep
  `EMMC_IRQn` off during the CMD18 stream (its handler clears BKGAP). Verify with numbered sectors (scratchpad
  `pattest.sh`), not just a checksum: the pattern tells which sector went where.
- Mode flags: product string "rp569 CH569 drive|serial" (`usb_product_string()`), link `\x01MDRV`/`\x01MSER` ->
  RP2350 "CH569 mode: ..." + `ch status ... mode X`; python `ports.ch569_usb_mode()`, `ch569_mode_events()`.
- SD benchmark: `PYTHONPATH=python python3 firmware/tools/sd_bench.py --mb 64 --runs 2 --note "..."` logs to
  `firmware/ch569/sd_benchmarks.csv` (firmware x.y.z, card CID); `--show` prints it. Console `sd` gives the CID.
- **Never test a new write path through the mounted volume**: the host writes the boot sector / FSINFO first
  (single-sector writes), a broken path destroys the filesystem (card MSSD0, 2026-11-19). Use raw writes to a
  test region at the end of the card. After a card swap, `ch cycle` before mounting (no SD_DET handling yet).
- RST#: works only with RESET_EN (set on board #1 via `firmware/tools/wch-ch56x-isp-resetpin.patch`, `-e=on`); a pin
  reset reloads the firmware, a software reset does not. SD_DET (PA20) low = card in, debounced in `sd_det_poll`.
- Never hardcode `/dev/ttyACM0`: the numbering changes with the enumeration order; use `/dev/serial/by-id/`
  (an RP2350 "hang" was the helper talking to the CH569 link port).
- Both speeds expose the same interfaces: `CH56x_usb20.c` (rewritten 2026-11-19) mirrors `CH56x_usb30.c`; a
  descriptor change goes in both, `build.sh check` compares them. CDC pumps (`cdc_uart.c`, `debug_cdc.c`) switch on
  `Link_Sta == LINK_STA_1` (USB2) and wait for `g_devInfo.dev_enum_status` there.
- UART1 stays at 115200 whatever the host sets (`LINK_FOLLOW_HOST_BAUD 0`): cdc_acm/ModemManager set 9600 on open.
- Flash script pattern: unmount the drive, `ch boot`, `wch-ch56x-isp -vr -f=...`; console replies on the console
  port (the boot log is dropped at USB reset, `debug_cdc_clear`).
- SD release: RP2350 `ch_sd_release()` sends `\x01SDREL\n` before every CH569 power-off and on `ch eject`; the CH569
  (`Link_Ctrl_Deal`, `cdc_uart.c`) releases the card once the MSC engine is idle (`ownership_poll` now never runs
  mid-transfer) and answers `\x01SDOFF\n`. Clean FAT needs the host unmount first: `python -m rp569 eject`
  (`python/rp569/drive.py`, udisksctl). The CLI helper must wait for the prompt: `ch off/cycle/boot` take up to 2 s more.
- SD traps (2026-11-19): multi-block CMD25 stalls after block 1 with TRAN_MODE bit 6 (keep it for n = 1 only) and
  needs `R32_EMMC_RESPONSE3 = 0` at each BKGAP; SD guards are per block; on a failed data phase send CMD12 or the card
  stays in receive-data. CMD6 status must go to its own zeroed buffer (reusing the SCR buffer gave stale / shifted
  data). `status` on the console ends with `sd clock N MHz` (48 = high speed). Speed test: unmount, write, sync,
  remount, sha256 (scratch `sdspeed.sh` pattern).
- Known: `chip_uid.c` ROM read returns stale `0xF7F9BF11` (serial `11BFF9F711BFF9F7`, real UID 8A82…E1); the
  ROM-access sequence matches HydraUSB3, `R8_GLOB_ROM_CFG` reads 0 and ignores a safe-mode write. Open in todo.
- Memory: the flasher sets ROM/RAMX from the image size (< 32 KB → RAMX 96 KB, < 64 → 64, < 96 → 32); `.DMADATA` uses
  86 KB of RAMX, so the image must stay < 32 KB unless the DMA buffers shrink. `tools/check_mem.py` (in `make` and
  `build.sh check`) enforces it; Link.ld's `RAMX 96K` cannot.
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
  touched component, staged automatically). **Never edit major/minor**: user only (minor = user releases, major = firmware variant, the same on both chips). `bump_versions.py --show`.
- Python tests: `cd python && python3 -m unittest discover -s tests -v` (no hardware, fake firmware). pytest is not
  installed; pyserial 3.5 is.

## Testing

- `PYTHONPATH=python python3 firmware/tools/link_test.py [-n 20]`: versions of both chips, then `ch send ping i`
  → expects the echo. `PYTHONPATH=python python3 -m rp569 ports` lists the port roles.
- Without hardware: both `build.sh` must finish without compiler warnings (the CH569 link prints
  `_close/_fstat/... not implemented` notes: expected). `firmware/ch569/build.sh check` must print "all good".
