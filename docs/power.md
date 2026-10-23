# Power architecture

Status: **draft v0.1 (2026-10-23)**. Part numbers are candidates. Check availability and datasheets
before the schematic.

## Block diagram

```
 USB-C (RP2350)  VBUS_C ──┬──[ideal diode D1]──┐
                          │                     ├── 5V_SYS ──[3V3 reg]── 3V3_RP ── RP2350B, flash, LA buffers,
 USB3 (CH569)    VBUS_3 ──┼──[ideal diode D2]──┘                                    LEDs, breakout headers
                          │
                          └──[load switch U_SW, EN = CH569_PWR_EN (RP2350 GPIO35)]── 5V_CH
                                                                                       │
                                                         ┌─────────────────────────────┤
                                                    [buck 3V3_CH]                 [buck 1V2_CH]
                                                         │                             │
                                                 CH569 VDDIO/V33*, SD card     CH569 V12CORE/V12USB
```

- **5V_SYS**: the two VBUS rails are ORed by ideal diodes, so the RP2350 runs from whichever connector
  is plugged in. Neither connector can back-feed the other: no current flows from VBUS_3 out of the
  USB-C, or from VBUS_C out of the USB3 connector.
- **5V_CH**: the CH569 is powered **only from its own USB3 VBUS**, through a load switch the RP2350
  controls. Switching it off and on power-cycles the CH569 (reset, bootloader entry; see D-004).
  The CH569 bucks' EN pins are tied to 5V_CH, as in phantomdrive.
- Optional jumper/0 Ω (DNP by default): 5V_SYS → 5V_CH switch input, so the CH569 can run without a
  USB3 cable (e.g. SD logging fed from USB-C). When fitted it bypasses D2's isolation on the CH569 side
  only. The ideal diodes still stop back-feed between the two connectors.

## Candidate parts

| Function | Candidate | Why | Notes |
|---|---|---|---|
| Ideal diode ×2 (D1, D2) | **TI LM66100** (SC-70-6) | 1.5 A, 5.5 V, ~79 mΩ, reverse-current blocking, CE pin | Alternative: TI TPS2116 power mux (2.5 A, priority mode) as a single part |
| CH569 load switch (U_SW) | **TI TPS2553** (SOT-23-6) | Adjustable current limit (up to ~1.5 A), FAULT# output, enable | Alternative: TPS22918 (no current limit, adjustable slew, quick discharge) |
| 3V3_CH, 1V2_CH bucks | **TI TLV62569DBV** ×2 | Same as phantomdrive (proven with CH569) | 2.2 µH, see `docs/ch569-hardware-notes.md` |
| 3V3_RP | RP2350 design-guide reference regulator | Follow `datasheet/hardware-design-with-rp2350.pdf` | RP2350 core rail: internal SMPS + its inductor |

The current limit and FAULT# make sense for test automation: a short on the CH569 side trips the switch,
and the RP2350 sees it on FAULT# instead of the whole board browning out.

## RP2350 power-control and monitoring signals

| RP2350B GPIO | Net | Notes |
|---|---|---|
| GPIO35 | CH569_PWR_EN | Load-switch EN. **Pull-down**: CH569 stays off until the RP2350 firmware enables it, which avoids back-powering during RP2350 boot |
| GPIO39 | CH569_PWR_FAULT# | TPS2553 FAULT# (open-drain, pull-up to 3V3_RP) |
| GPIO46 (ADC6) | VBUS3_SENSE | VBUS_3 through a divider (e.g. 100k/47k → ~1.6 V at 5 V): USB3 cable present + level |
| GPIO47 (ADC7) | CH569_3V3_SENSE | 3V3_CH through a divider: confirms the CH569 is really powered / off |

## Rules

- When 5V_CH is off, the RP2350 tri-states every signal going to the CH569, except HSPI_D0, which may be
  held low (boot selection, no back-power through a low level). Series resistors on the inter-chip lines
  limit any back-feed current through the CH569's ESD diodes.
- Pull-ups on the CH569 side (SD card, RST#, CH569 I2C) go to **3V3_CH**, not 3V3_RP.
- The shared I2C bus: pull-ups on 3V3_RP. While the CH569 is off its pins must not clamp the bus, so
  check the CH569 I/O structure, or add a small isolating buffer if it does (TODO).
