---
title: "rp569: first firmware for two chips that boot each other"
status: draft (version/UID values in the examples are placeholders until boards exist)
target: hackaday.io project log
images: ../images/rp569-v1.0-top.png (RPDFU and BOOT buttons, RGB LEDs); ideally a terminal screenshot of `help`
---

# rp569: first firmware for two chips that boot each other

The board isn't fabricated yet, but both chips now have firmware that builds. The goal of this first round is not
speed yet: it is *control*. Before streaming anything over USB 3, I want the RP2350B to be able to switch the
CH569 on and off, force it into its bootloader, and talk to it, so the CH569 can be reflashed and tested from a
script without anyone pressing a button.

The code is in [`firmware/`](https://github.com/kelu124/rp569/tree/main/firmware): `rp2350/` and `ch569/`, each
with its own `build.sh`.

## RP2350B: a command line on the USB port

The RP2350B side uses the Pico SDK 2.2 with a board header for the rp569 (`boards/rp569.h`: RP2350B, 16 MB flash,
UART0 on GPIO32/33 and every pin of the board as an `RP569_*` define). The USB serial port gives a small command
line:

```
rp569> help
  version                  firmware version, git hash and chip unique ID
  set <R|G|B> <0|1>        RGB LED colour on/off (also 'set rgb 101')
  dfu                      reboot the RP2350 into its USB bootloader (BOOTSEL / UF2)
  ch on / off / cycle      CH569 power
  ch boot                  power-cycle the CH569 into its USB bootloader (~10 s)
  ch status                CH569 power, fault flag, rails
  ch send <text>           send a line to the CH569 over the link UART (it echoes it back)
  ...
```

`dfu` is one call, `reset_usb_boot(0, 0)`: the RP2350 comes back as its UF2 drive, ready for the next build, and
the BOOTSEL button (labelled **RPDFU** on the silkscreen) is only needed for a blank chip.

### Powering and booting the CH569 from the RP2350

The CH569 is fed through a load switch (AP2171W) whose enable has a pull-down: the CH569 stays off until the
RP2350 drives GPIO29 high. That makes the boot strap easy to control:

1. `ch boot` drives GPIO8 low. It is HSPI data line 0 on the RP2350, and it lands on the CH569's PA5, its boot pin.
2. Power off, wait until the CH569's 3.3 V rail (read by the RP2350's ADC through a divider) has gone down.
3. Power on: the CH569 samples PA5 low and starts the WCH USB bootloader on the USB3 connector for ~10 s.
4. Release GPIO8 200 ms later; it is a data line again.

Two details worth knowing if you copy this:

- **No back-powering**: while the CH569 is off, every RP2350 pin going to it is an input *without* pull-up. A UART
  TX idling high into an unpowered chip happily powers it through its protection diodes.
- **The jumper must still win**: on a normal power-up GPIO8 only gets the RP2350's weak pull-up, so the CH BOOT
  jumper (1 kΩ to GND) can still force the bootloader on a blank chip.

`ch reset` pulses the CH569's RST#, but that pin only works once the RESET_EN configuration bit is set in the chip.
Power cycling always works, so that is what `ch cycle` does.

## CH569W: USB3 serial ports and an echo

For the CH569 I did not start from scratch: I forked my
[CH569_Composite](https://github.com/kelu124/CH569_Composite) firmware. It enumerates as a USB 3.0 composite
device: mass storage on the microSD card, plus two CDC serial ports. On rp569:

- **Serial port 1** is bridged to UART1 (PA7/PA8), the link to the RP2350. Every byte the RP2350 sends is
  **echoed back** to it, and copied to the host. So `ch send hello` on the RP2350 prints `CH> hello` a few ms
  later: the whole loop (RP2350 UART → CH569 → back) works without a PC on the CH569 side.
- **Serial port 2** is the debug log (also on UART0, the CH UART header), now with a tiny console: `help`,
  `version`, `status`.
- The **microSD** shows up as a drive, mounted at power-up (no SD mux on this board).
- The **RGB LED** shows SD ready (R), USB3 link up (G), and toggles on each line from the RP2350 (B).

Porting it found two bugs in the vendor SDK's `PRINT` macro. It always waited on UART1's "transmit empty" flag,
whatever UART the debug output used, and on rp569 UART1 is the RP2350 link. And it expanded to two statements,
so `for(...) PRINT(...)` only looped the `printf`. Both are fixed in the fork.

One limitation inherited from the WCH stack: the serial ports only exist on a SuperSpeed link. On a USB 2 port, the
CH569 shows only the drive.

## Versions you can trust

Each firmware has a `version.yaml` (major.minor.patch) compiled in together with the `git describe` hash, and the
`version` command prints it with the chip's unique ID:

```
rp569 RP2350B firmware 0.1.0 git 0727cb0 built Oct 24 2026 uid E6614103E7A1B22C
rp569 CH569W firmware 0.1.0 git 0727cb0 built Oct 24 2026 uid 0123456789ABCDEF
```

The RP2350's ID comes from `pico_get_unique_board_id`. The CH569 has 8 factory-programmed bytes at flash address
0x77FE4, read the way HydraUSB3 does it. Both IDs are also the USB serial numbers, so a host script can tell boards
apart. A git pre-commit hook bumps the patch number of every firmware a commit touches. Major and minor stay
manual.

## Next

Boards first. Then the real work: a PIO program that clocks 8 bits out on GPIO8–15 with the HSPI handshake, and
the CH569 side receiving it into USB3 bulk transfers.
