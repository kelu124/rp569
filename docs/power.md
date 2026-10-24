# Power architecture

Status: **v1.0 (2026-10-24)**, as built in `hardware/gen/board.py`. Parts are in the [BOM](bom.md); design
decisions D-004, D-007, D-009, D-015 in
[specification.md](https://github.com/kelu124/rp569/blob/main/specification.md).

## Block diagram

```
 USB-C (RP2350)  VBUS_C ──┬──[TPS2121 IN1]─────┐
                          │                     ├── 5V_SYS ──[TLV62569]── +3V3 ── RP2350B, flash,
 USB3 (CH569)    VBUS_3 ──┼──[TPS2121 IN2]─────┘                                    LEDs, breakout headers
                          │
                          └─[0 Ω]─[AP2171W, EN = CH569_PWR_EN (RP2350 GPIO29)]── 5V_CH
                                                                                       │
                                                         ┌─────────────────────────────┤
                                                    [buck 3V3_CH]                 [buck 1V2_CH]
                                                         │                             │
                                                 CH569 VDDIO/V33*, SD card     CH569 V12CORE/V12USB
```

- **5V_SYS**: the two VBUS rails are ORed by a TI TPS2121 power mux (ideal-diode behaviour, D-015), so the RP2350 runs from whichever connector
  is plugged in. Neither connector can back-feed the other: no current flows from VBUS_3 out of the
  USB-C, or from VBUS_C out of the USB3 connector.
- **5V_CH**: the CH569 is powered **only from its own USB3 VBUS**, through a load switch the RP2350
  controls. Switching it off and on power-cycles the CH569 (reset, bootloader entry; see D-004).
  The CH569 bucks' EN pins are tied to 5V_CH, as in phantomdrive.
- Optional 0 Ω (DNP by default): 5V_SYS → 5V_CH switch input, so the CH569 can run without a USB3 cable
  (e.g. SD logging fed from USB-C). Fit it **instead of** the VBUS_3 0 Ω, not with it, or VBUS_3 and 5V_SYS
  are shorted around the TPS2121. The TPS2121 still stops back-feed between the two connectors.

## Parts

| Function | Candidate | Why | Notes |
|---|---|---|---|
| Power mux (U2) | **TI TPS2121RUXR** (VQFN-HR-12), D-015 | 2.8–22 V, 56 mΩ, reverse-current blocking, OV supervisor per input, current limit; at JLCPCB (C485916) | Replaced the LM66100 ×2 (not stocked by JLCPCB). VCOMP mode: higher input wins; OV at ~6.1 V; ILIM 100k ≈ 1.2 A |
| CH569 load switch (U3) | **Diodes AP2171W** (SOT-23-5), D-009 | 1 A current limit, FLG# output, active-high EN | Alternative: TI TPS2553 (adjustable limit up to ~1.5 A) |
| 3V3_CH, 1V2_CH bucks | **TI TLV62569DBV** ×2 | Same as phantomdrive (proven with CH569) | 2.2 µH, see [CH569 notes](ch569-hardware-notes.md) |
| +3V3 (RP2350 side) | **TI TLV62569DBV** from 5V_SYS | Same part as the CH569 bucks | 453k/100k feedback |
| 1V1 (RP2350 core) | RP2350 internal SMPS | RP2350 hardware design guide §2.1 | 3.3 µH inductor, 4.7 µF caps |

The current limit and FAULT# make sense for test automation: a short on the CH569 side trips the switch,
and the RP2350 sees it on FAULT# instead of the whole board browning out.

## RP2350 power-control and monitoring signals

| RP2350B GPIO | Net | Notes |
|---|---|---|
| GPIO29 | CH569_PWR_EN | Load-switch EN. **Pull-down**: CH569 stays off until the RP2350 firmware enables it, which avoids back-powering during RP2350 boot |
| GPIO30 | CH569_PWR_FAULT# | AP2171W FLG# (open-drain, 10k pull-up to +3V3) |
| GPIO46 (ADC6) | VBUS3_SENSE | VBUS_3 through 100k/47k (→ ~1.6 V at 5 V), divider at the USB3 connector (D-020) |
| GPIO47 (ADC7) | CH3V3_SENSE | 3V3_CH through 100k/100k, divider at the 3V3_CH inductor: CH569 really on / off |

## Rules

- When 5V_CH is off, the RP2350 tri-states every signal going to the CH569, except HSPI_D0, which may be
  held low (boot selection, no back-power through a low level). The HSPI lines have no series resistors
  (D-018), so this is a firmware rule: an RP2350 output driven high into an unpowered CH569 would back-feed it
  through its ESD diodes.
- Pull-ups on the CH569 side (SD card, RST#, CH569 I2C) go to **3V3_CH**, not +3V3.
- The shared I2C bus: pull-ups on +3V3. While the CH569 is off its pins must not clamp the bus, so
  check the CH569 I/O structure, or add a small isolating buffer if it does (TODO).
