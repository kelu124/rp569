---
title: "rp569: a Python library that knows which serial port is which"
status: draft (version/UID values in the examples are placeholders until boards exist)
target: hackaday.io project log
images: a terminal screenshot of `python3 -m rp569 ports` once boards exist
---

# rp569: a Python library that knows which serial port is which

Plug in an rp569 board and you get three serial ports: the RP2350B command line, the CH569's bridge to the
RP2350 and the CH569's console. On Linux that is `/dev/ttyACM0..2`, in whatever order the kernel felt like. Add a
second board, or the Pico on the bench next to it, and "open ttyACM0" becomes a coin toss. A test script that
sends `ch boot` to the wrong chip is not a good test script.

So the host side of rp569 is a small Python library,
[`python/rp569`](https://github.com/kelu124/rp569/tree/main/python), built on pyserial, that finds each port by
what it *is*.

## Finding the ports

Each port gets a role from its USB descriptors:

| Role | How it is recognised |
|---|---|
| `rp2350` | VID/PID `2e8a:0009` (Pico SDK CDC) **and** product string "rp569 RP2350B" |
| `ch569-link` | `1a86:fe2c` **and** product "rp569 CH569", interface 1 |
| `ch569-console` | same device, interface 3 |

The product strings are set by the firmware (a compile definition on the Pico side, the string descriptor on the
CH569), so a stock Pico running something else is not mistaken for the board. The two CH569 ports belong to the
same USB device; they differ by interface number, which pyserial exposes in the port's location (`2-1:1.1` vs
`2-1:1.3` on Linux). On systems that don't show it, they are ordered by device name.

Both firmwares use the **chip's unique ID as the USB serial number**: the RP2350's board ID, and the CH569's 8
factory bytes. That makes `RP2350(serial_number='E661...')` pick one board out of several. If two match and no ID
is given, the library refuses and lists them instead of guessing.

Three checks keep a class off the wrong chip:

1. An explicit port of another role (`RP2350('/dev/ttyACM1')` when ACM1 is the CH569 link) raises `WrongDevice`.
2. After opening, the `version` reply must name the expected chip (`RP2350B` / `CH569W`).
3. Discovery is strict by default; `strict=False` is there for a stock Pico firmware or older CH569 builds.

```
$ python3 -m rp569 ports
rp2350         /dev/ttyACM0     serial E6614103E7A1B22C  product 'rp569 RP2350B'  if 0
ch569-link     /dev/ttyACM1     serial 0123456789ABCDEF  product 'rp569 CH569'    if 1
ch569-console  /dev/ttyACM2     serial 0123456789ABCDEF  product 'rp569 CH569'    if 3
```

## One method per firmware command

```python
import rp569

with rp569.RP2350() as rp:
    print(rp.version())          # FirmwareVersion(chip='RP2350B', major=0, minor=1, patch=0, git=..., uid=...)
    rp.set_led('G', True)
    print(rp.adc())              # {'VBUS3': 5.02, '3V3_CH': 3.31}
    rp.ch_boot()                 # CH569 into its bootloader: flash it now
    rp.ch_cycle()
    assert rp.ch_send('ping') == 'ping'    # the CH569 echoes it back

with rp569.CH569Console() as con:
    print(con.status())          # {'usb': 'usb3', 'sd_ready': True, ...}
```

Reading a human-oriented command line from a script has one trap: unsolicited lines. The RP2350 prints anything
the CH569 sends as `CH> ...`, at any time, and then redraws its prompt. The library treats a reply as done only
when it has seen the echo of *its* command followed by a bare prompt, and keeps the `CH>` lines aside
(`rp.ch_lines()`). The unit tests run against a fake firmware that injects such lines before the echo, since that
is where the first version broke.

## Keeping the library and the firmware in sync

The commands live in one table per firmware, between `COMMANDS-BEGIN` and `COMMANDS-END` markers. The RP2350's
`help` is generated from it. In Python, every method that wraps a command is marked `@command("ch boot")`. A
pre-commit hook (`tools/check_commands.py`) reads both sides (regex on the C table, `ast` on the Python, nothing
imported) and refuses the commit if a command exists on one side only. Add `blink` to the CH569 table without a
`blink()` method and the commit stops:

```
CH569W: firmware command 'blink' (firmware/ch569/src/console.c) has no @command('blink') method in python/rp569/ch569.py
```

The same hook bumps the patch version of every component the commit touches: the two firmwares (`version.yaml`)
and the library (`rp569.__version__`). Major and minor are left to me. So any `version()` reply points at one
commit.

## Next

Once boards are back: `firmware/tools/link_test.py` runs the versions and 20 pings through the library, and the
same library will drive the HSPI streaming tests.
