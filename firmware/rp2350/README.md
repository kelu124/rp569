# RP2350B firmware

USB serial command line (also mirrored on the debug UART0, RP EXT header pins 2 TX / 3 RX, 115200 8N1).

```
rp569> help
  set <R|G|B> <0|1>    RGB LED colour on/off (also 'set rgb 101')
  led                  LED state
  btn                  user buttons
  adc                  VBUS3 and 3V3_CH voltages
  dfu                  reboot the RP2350 into its USB bootloader (BOOTSEL / UF2)
  reboot               reboot the RP2350
  ch on|off|cycle      CH569 power (cycle = off, then normal boot)
  ch boot              power-cycle the CH569 into its USB bootloader (wch-ch56x-isp, ~10 s)
  ch reset             pulse RST# (only with RESET_EN set in the CH569)
  ch status            power, fault flag, rails
  ch send <text>       send a line to the CH569 over the link UART (it echoes it back)
  version              firmware version
```

Lines from the CH569 (link UART1, GPIO24 TX / GPIO25 RX, 115200) are printed as `CH> ...`.

## Build

```sh
./build.sh            # PICO_SDK_PATH defaults to ~/.pico-sdk/sdk/2.2.0 -> build/rp569_rp2350.uf2
```

Needs Pico SDK ≥ 2.0 (RP2350), `arm-none-eabi-gcc`, cmake, ninja. The board definition is
[`boards/rp569.h`](boards/rp569.h): RP2350B (`PICO_RP2350A 0`), 16 MB flash, UART0 on GPIO32/33, every rp569 pin
as `RP569_*`.

## CH569 control (specification.md D-004)

- The CH569 is **off** until GPIO29 (AP2171W EN, 100k pull-down) goes high; the firmware turns it on at start-up
  (`CH_POWER_ON_BOOT`).
- While it is off, the link UART pins and HSPI_D0 are inputs without pull-up (no back-powering).
- `ch boot`: GPIO8 (HSPI_D0 → CH569 PA5) driven low, power off, power on, released 200 ms later.
- Normal power-up: GPIO8 only gets the weak pull-up, so the CH BOOT jumper/button still force the bootloader.
- `ch reset` pulses GPIO28 (open-drain RST#); it only works once RESET_EN is set in the CH569 config bits.
- GPIO30 (FLG#) is polled: a power-switch fault is reported once.
