# rp569

An open hardware board (KiCad) that pairs a **Raspberry Pi RP2350B** with a **WCH CH569W**.
The RP2350B generates high-speed data and the CH569 streams it to a host over **USB 3.0**.

Status: **early design**. Requirements and reference material are being gathered. No schematic yet.

## Features (planned)

| Block | RP2350B | CH569W |
|-------|---------|--------|
| USB | USB 2.0 on USB-C (firmware, debug, test control) | USB 3.0 SuperSpeed connector |
| Storage | QSPI flash | SD card on the eMMC/SD controller |
| User I/O | RGB LED | RGB LED |
| Breakout | I2C header for extra devices | I2C header (bit-banged GPIO) |

**Logic analyser**: 8 input pins on the RP2350B are sampled by PIO and streamed over the
parallel bus to the CH569, then to the host over USB 3.

**Links between the two chips**
- 8-bit parallel bus with clock and handshake lines (8-bit because the CH569's 16-bit mode shares pins with the SD card; see `.claude/decisions.md` D-003): RP2350B PIO sends, CH569 HSPI receives.
- Shared I2C bus.
- UART.
- CH569 RESET# and boot-mode pins driven by the RP2350B, so the CH569 can be reflashed and
  power-cycled automatically during testing.

## Repository layout

| Path | Content |
|------|---------|
| `hardware/` | KiCad project (to come) |
| `datasheet/` | Datasheets as PDF and Markdown (see `datasheet/README.md`) |
| `docs/` | `pinmap.md` (RP2350B ↔ CH569 pin map), `ch569-hardware-notes.md` |
| `project/` | Reference projects: [HydraUSB3](https://github.com/hydrausb3), [phantomdrive](https://github.com/o7-machinehum/phantomdrive) (see `project/README.md`) |
| `CLAUDE.md`, `.claude/` | Project context, todo/done logs, commit log, design decisions |

## References

- [HydraUSB3](https://github.com/hydrausb3): open-source CH569 dev kit, firmware and HSPI/USB3 drivers.
- [phantomdrive](https://github.com/o7-machinehum/phantomdrive): open-source CH569W encrypted USB3 drive with SD card, KiCad sources.
- [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf) and
  [hardware design guide](https://datasheets.raspberrypi.com/rp2350/hardware-design-with-rp2350.pdf).
