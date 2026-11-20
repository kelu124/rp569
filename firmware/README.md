# rp569 firmware

First firmware for the two chips of the board. Both build on this machine; neither has run on hardware yet
(no board fabricated). Pins come from `hardware/gen/board.py` / [`docs/pinmap.md`](../docs/pinmap.md).

| Folder | Chip | What it does | Build | Output |
|---|---|---|---|---|
| [`rp2350/`](rp2350/) | RP2350B | USB serial command line: RGB LED, buttons, ADC, `dfu`, CH569 power/boot/reset, messages to the CH569 | `rp2350/build.sh` (Pico SDK 2.2) | `rp2350/build/rp569_rp2350.uf2` |
| [`ch569/`](ch569/) | CH569W | USB3 composite: serial port bridged to the RP2350 link UART (echoes every byte back to the RP2350), SD card as a drive, debug log port | `ch569/build.sh` (xPack riscv-none-elf-gcc) | `ch569/build/rp569_ch569.bin` |
| [`tools/`](tools/) | host | `link_test.py`: versions of both chips, then pings the CH569 through the RP2350 CLI and checks the echo | `PYTHONPATH=python python3 firmware/tools/link_test.py` | |

Host side: the python library in [`python/`](../python/) (port discovery by VID/PID/product/UID, one method per
command).

## Versions

Each firmware has a `version.yaml` (major.minor.patch) compiled in and printed by its `version` command, with the
git hash and the chip unique ID. **major/minor are changed by the user only**; the pre-commit hook
(`.githooks/pre-commit`, enable once with `tools/install_hooks.sh`) bumps the patch of every component a commit
touches (`tools/bump_versions.py`, also for `python/`) and refuses a commit where a firmware command and the python
library disagree (`tools/check_commands.py`).

## Firmware variants (major version = variant, D-025)

| Variant | CH569 | RP2350 | What it does |
|---|---|---|---|
| 0 (0.x) | `./build.sh` -> `build/rp569_ch569.bin` | `build/rp569_rp2350.uf2` | drive / serial modes, SD card (D-028..D-030) |
| 1 (1.x) "stream" | `./build.sh VARIANT=stream` -> `build-stream/rp569_ch569_stream.bin` | `build/rp569_rp2350_stream.uf2` | RP2350 counter stream over the 8-bit HSPI bus -> CH569 -> USB bulk, 16 MB/s, max 44.7 MB/s (D-031, D-033) |

Variant 1 test: flash both, `ch cycle`, then `PYTHONPATH=python python3 -m rp569 stream 20 16` (needs pyusb, numpy
and USB access to 1a86:fe2c). RP2350 CLI: `stream start [MB/s] [MHz] [crc]` (16, 25, 8 = CRC-16/USB),
`stream stop`, `stream status`. Maximum speed: `stream start 0 75 9` (unthrottled, 75 MHz bus, crc 9 = fast CRC
of the counter pattern): 44.7 MB/s at the host, the USB2 bulk ceiling (D-033).

## Bring-up order

1. **RP2350**: hold **RPDFU** (BOOTSEL) and press **RUN** (or plug in with RPDFU held): the RP2350 appears as a
   USB drive. Copy `rp2350/build/rp569_rp2350.uf2` to it, or `picotool load -x rp2350/build/rp569_rp2350.uf2`.
   Later updates: type `dfu` in the CLI, or `picotool load -f -x ...` (the firmware keeps picotool's reset interface).
2. Open the RP2350 serial port (`/dev/ttyACM0`, 115200, any terminal: `picocom /dev/ttyACM0`), type `help`.
   `set R 1` / `set R 0` drive the red LED, `ch status` shows the CH569 rails.
3. **CH569**: plug the USB3 cable (the CH569 is fed from the USB3 VBUS), type `ch boot` in the RP2350 CLI: the
   CH569 restarts in the WCH USB bootloader (`4348:55e0`) for ~10 s. Flash with
   `wch-ch56x-isp -v -f ch569/build/rp569_ch569.bin` (HydraUSB3 tool) or `wchisp flash ch569/build/rp569_ch569.bin`.
   Then `ch cycle`. A blank CH569 can also be forced with the CH BOOT jumper or the BOOT button at power-up.
4. The CH569 enumerates as `1a86:fe2c` with two serial ports (RP2350 link, debug log) and a drive (microSD).
5. Link test: `ch send hello` in the RP2350 CLI prints `CH> hello`; the CH569 link port on the host shows `hello`.
   `PYTHONPATH=python python3 firmware/tools/link_test.py` prints both versions and runs 20 pings.
   `PYTHONPATH=python python3 -m rp569 ports` shows which serial port is which.

The CH569 serial ports exist only on a USB3 (SuperSpeed) link: on a USB2 port or cable the CH569 shows only the
drive (vendor USB2 stack, see `ch569/README.md`). The RP2350-side echo works in both cases.
