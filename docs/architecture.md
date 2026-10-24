# Architecture

How the two chips, the buses, the power and the boot control fit together. Pin-level detail is in the
[pin map](pinmap.md), power in [power](power.md); the reasons for each choice are the D-xxx entries of
[specification.md](https://github.com/kelu124/rp569/blob/main/specification.md).

## Block diagram

```
                 USB-C (USB 2.0)                                   USB 3.0 Micro-B
                       │ D+/D-                                           │ SS TX/RX, D+/D-
                       │                                                 │
 LA IN 1x10 ──100Ω──► ┌┴──────────────────────┐   HSPI 8-bit + ctrl   ┌──┴────────────────────┐ ◄──► microSD (4-bit)
 (8 inputs)           │      RP2350B          │ ───────────────────►  │       CH569W          │
                      │  QFN-80, 48 GPIO      │   GPIO8-23 ↔ HD0-7,   │  RISC-V, USB3 SS,     │
 QSPI flash 16 MB ◄──►│  3 × PIO              │   RCLK/RACT/RVLD/...  │  HSPI, eMMC/SD        │ ◄──► CH I2C header
                      │                       │ ◄── UART ──────────►  │                       │ ◄──► CH EXT 2x07
 RP I2C header ◄────► │                       │ ◄── I2C (shared) ──►  │                       │ ◄──► RGB LED, 2 buttons
 RP EXT 1x10   ◄────► │                       │ ─── RST#, boot (HD0)►  │                       │
 RGB LED, 2 buttons ◄►│                       │ ─── PWR_EN ──► load   │                       │
 SWD header ◄───────► └───────────────────────┘     switch ─── 5V_CH ►└───────────────────────┘
```

## The data path

- **RP2350B → CH569 over HSPI, 8 bits.** The CH569's HSPI is the receiver: 8 data lines (HD0–HD7) plus HRCLK,
  HRACT, HRVLD and HTACK. The CH569 transmit-side control lines are wired too, for a later reverse direction.
  On the RP2350 the 16 lines are GPIO8–23, contiguous for PIO and placed on the QFN side that faces the CH569.
  The lines run straight, 2–3 cm, without series resistors (D-018).
- **Why 8 bits:** the CH569's HSPI data lines above HD7 share pins with its eMMC/SD controller. 16 bits would cost
  the SD card (D-003).
- **CH569 → host over USB 3.0 SuperSpeed** on the Micro-B connector. The connector's USB 2.0 lines go to the CH569
  too: they carry its USB bootloader.

## The logic analyser

GPIO0–7 come out on the LA IN header (8 inputs + 2 GND) through 100 Ω series resistors. They are consecutive so a
PIO program can do `in pins, 8`, and they sit in the same 32-GPIO PIO window as the HSPI pins (GPIO0–23), so one
PIO block can sample the inputs and drive the bus.

## Control links

| Link | RP2350B | CH569 | Use |
|---|---|---|---|
| UART | UART1 GPIO24 (TX) / 25 (RX) | UART1 PA7 / PA8 | Commands, logs |
| I2C (shared) | I2C1 GPIO26 / 27 | PA14 / PA15 (bit-banged) | Configuration, status |
| Reset | GPIO28 (open-drain) | RST# | Only active when the CH569 config bit RESET_EN is set |
| Power | GPIO29 → AP2171W EN, GPIO30 ← FLG# | — | Power-cycle the CH569, detect over-current |
| Boot | HSPI_D0 (GPIO8) | PA5 / HD0 | Low at power-up → CH569 USB bootloader |
| Monitoring | GPIO46 / 47 (ADC) | — | USB3 VBUS and 3V3_CH levels through dividers |

## Automated reflashing of the CH569

1. The RP2350 turns the CH569 off (CH569_PWR_EN low) and holds HSPI_D0 low.
2. It turns the CH569 on: the CH569 samples PA5 low and starts its USB bootloader (`4348:55E0`) on the USB3
   connector's USB 2.0 lines.
3. The host flashes it (`wch-ch56x-isp`), then the RP2350 releases HSPI_D0 and power-cycles the CH569 again.

The BOOT button does the same by hand, and the CH BOOT JP jumper holds PA5 low permanently for a blank chip.
The bootloader times out after ~10 s without activity.

## Power

```
 USB-C VBUS_C ──► TPS2121 IN1 ┐
                              ├─► 5V_SYS ─► TLV62569 ─► +3V3 ─► RP2350B (its internal SMPS makes 1V1), flash, LEDs, headers
 USB3  VBUS_3 ──► TPS2121 IN2 ┘
       VBUS_3 ──► 0 Ω ─► AP2171W (EN = GPIO29, FLG# → GPIO30) ─► 5V_CH ─┬─► TLV62569 ─► 3V3_CH ─► CH569 I/O, SD card
                                                                       └─► TLV62569 ─► 1V2_CH ─► CH569 core / USB
```

- The TPS2121 picks the higher input, blocks back-feed between the two connectors, cuts off above ~6.1 V and
  limits to ~1.2 A.
- The CH569 runs only from the USB3 VBUS by default; a DNP 0 Ω can feed its switch from 5V_SYS instead.
- While the CH569 is off, the RP2350 tri-states every line to it except HSPI_D0 (no back-powering).

Details: [power](power.md).

## Clocks

Each chip has its own crystal: 12 MHz for the RP2350B, 30 MHz for the CH569 (its USB3 PLL reference). The only
shared clock is the HSPI data clock, driven by the RP2350.

## PCB

4 layers (JLC04161H-7628, 1.6 mm), 50 × 70 mm, 3 mm rounded corners, parts on both sides, both chips on top. In1 is
the GND plane under the 90 Ω USB pairs on F.Cu; In2 and B.Cu carry the rest. See [layout and routing](layout.md)
and the [images](images/).
