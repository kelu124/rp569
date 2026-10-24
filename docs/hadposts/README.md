# Hackaday drafts

Drafts for hackaday.io project logs about rp569. Facts come from `specification.md`, `docs/` and the repo history;
check numbers and the repository URL before publishing. Images: `docs/images/` (3D renders, layer plots as PNG for
upload); `docs/img/` holds older renders (v0.2) and figures.

| File | Topic | Status |
|---|---|---|
| [01-rp2350b-meets-ch569.md](01-rp2350b-meets-ch569.md) | What the board is: RP2350B + CH569, HSPI, logic analyser, RP2350 controlling the CH569 | draft |
| [02-autorouting-a-dense-4-layer-board.md](02-autorouting-a-dense-4-layer-board.md) | Routing story: Freerouting pitfalls, KiCadRoutingTools, placement lessons, 421 → 0 | draft |
| [03-first-firmware-for-two-chips.md](03-first-firmware-for-two-chips.md) | RP2350B CLI (`dfu`, CH569 power/boot), CH569 USB3 serial + echo, versions and chip UIDs | draft |
| [04-a-python-library-that-knows-which-port-is-which.md](04-a-python-library-that-knows-which-port-is-which.md) | Host library: port roles by VID/PID/product/UID/interface, one method per command, sync and version hooks | draft |

Ideas for later logs: the generated KiCad workflow (one Python file → schematic, PCB, BOM); JLCPCB parts hunting
without the LCSC API (jlcparts data); bring-up and first HSPI transfer once boards arrive.
