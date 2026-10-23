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

**Links between the two chips**
- 16-bit parallel bus with clock and handshake lines: RP2350B PIO sends, CH569 HSPI receives.
- Shared I2C bus.
- UART.
- CH569 RESET# and boot-mode pins driven by the RP2350B, so the CH569 can be reflashed and
  power-cycled automatically during testing.

## Repository layout

| Path | Content |
|------|---------|
| `hardware/` | KiCad project (to come) |
| `datasheet/` | Datasheets as PDF and Markdown (to come) |
| `docs/` | Pin maps, block diagrams, design notes (to come) |
| `project/` | Reference projects, e.g. [HydraUSB3](https://github.com/hydrausb3) (see `project/README.md`) |
| `CLAUDE.md`, `.claude/` | Project context, todo/done logs, commit log, design decisions |

## References

- [HydraUSB3](https://github.com/hydrausb3): open-source CH569 dev kit, firmware and HSPI/USB3 drivers.
- [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf) and
  [hardware design guide](https://datasheets.raspberrypi.com/rp2350/hardware-design-with-rp2350.pdf).
