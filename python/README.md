# rp569 python library

Host library for the rp569 board. It finds each serial port by USB VID/PID, product string, serial number (=
chip unique ID) and interface number, so a class never opens the other chip's port. It has one method per firmware
command.

```sh
pip install -e python        # or: PYTHONPATH=python python3 ...
python3 -m rp569 ports       # list the rp569 ports and their roles
python3 -m rp569 versions    # firmware version + chip UID of every chip found
```

```python
import rp569
print(rp569.__version__)

with rp569.RP2350() as rp:                  # 2e8a:0009, product "rp569 RP2350B"
    print(rp.version())                     # FirmwareVersion(chip='RP2350B', major=0, minor=1, patch=3, git=..., uid=...)
    rp.set_led('R', True); rp.set_rgb(0, 1, 0)
    print(rp.adc(), rp.buttons(), rp.ch_status())
    print(rp.ch_send('hello'))              # 'hello': echoed back by the CH569

with rp569.CH569Console() as con:           # 1a86:fe2c, product "rp569 CH569", interface 3
    print(con.version(), con.status())

with rp569.CH569Link() as link:             # 1a86:fe2c, interface 1: raw RP2350 link
    link.write_line('to the RP2350')         # shows up in the RP2350 CLI as "CH> to the RP2350"
```

| Port role | USB | Class |
|---|---|---|
| `rp2350` | 2e8a:0009 "rp569 RP2350B" | `RP2350` (command line, prompt `rp569> `) |
| `ch569-link` | 1a86:fe2c "rp569 CH569", interface 1 | `CH569Link` (raw lines, no command line) |
| `ch569-console` | 1a86:fe2c "rp569 CH569", interface 3 | `CH569Console` (debug log + console, prompt `ch569> `) |

Safeguards: an explicit port of another role raises `WrongDevice`; after opening, the `version` reply must name the
expected chip; several boards → pass `serial_number=<uid>` (`PortNotFound` lists them). `strict=False` also accepts
a stock Pico SDK CDC / the original CH569_Composite product strings.

## Commands stay in sync with the firmware

Each method that wraps a firmware command is marked `@command("<name>")`. `tools/check_commands.py` compares those
names with the firmware command tables (`firmware/rp2350/src/main.c`, `firmware/ch569/src/console.c`, between
`COMMANDS-BEGIN` / `COMMANDS-END`). The pre-commit hook runs it: a commit that adds, renames or removes a command
on one side only is refused.

## Version

`rp569.__version__` (`rp569/_version.py`), major.minor.patch. The pre-commit hook bumps the patch on every commit
that changes `python/`; major/minor are changed by the user only.

## Tests

```sh
cd python && python3 -m unittest discover -s tests -v     # no hardware: fake ports and a fake firmware
```
