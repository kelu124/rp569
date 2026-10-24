# Production (JLCPCB)

How to order the v1.0 board, fabricated and assembled by JLCPCB. Background: [bill of materials](bom.md),
[layout and routing](layout.md).

## Files

| File | Where it goes |
|---|---|
| [rp569-gerbers.zip](https://github.com/kelu124/rp569/raw/main/hardware/jlcpcb/rp569-gerbers.zip) | PCB order → *Add gerber file* |
| [rp569-bom.csv](https://github.com/kelu124/rp569/blob/main/hardware/jlcpcb/rp569-bom.csv) | PCB Assembly → BOM |
| [rp569-cpl.csv](https://github.com/kelu124/rp569/blob/main/hardware/jlcpcb/rp569-cpl.csv) | PCB Assembly → CPL (pick and place) |

All three come from the routed board `hardware/rp569-routed.kicad_pcb`:

```bash
python3 hardware/gen/gen_fab.py
```

The script runs a DRC first and writes nothing unless there are 0 unconnected items and 0 errors. The gerbers use
the settings of JLCPCB's KiCad guide: Protel file extensions, no X2/netlist attributes, soldermask subtracted
from silkscreen, Excellon drill in mm with decimal format, absolute origin, separate PTH/NPTH files.
The unzipped files are in `hardware/jlcpcb/gerbers/` for a look in KiCad's Gerber viewer.

## PCB order settings

| Setting | Value |
|---|---|
| Layers | 4 |
| Dimensions | 50 × 70 mm |
| Thickness | 1.6 mm |
| Stackup | JLC04161H-7628 (the 90 Ω USB pair width/gap 0.24/0.13 mm was computed for it) |
| Min track / clearance used | 0.15 / 0.1 mm (JLCPCB standard) |
| Min via | 0.5 mm pad / 0.3 mm drill (no extra cost) |
| Impedance control | optional; confirm 90 Ω first with JLCPCB's impedance calculator |

## Assembly

- **Both sides** are populated (decoupling capacitors and many passives are on the bottom).
- **Two parts are not stocked by JLCPCB** and must be consigned (bought elsewhere and sent to JLCPCB) or soldered
  by hand: the **WCH CH569W** (QFN-68) and the **Amphenol GSB343K33HR** USB3 Micro-B connector.
- The 2.54 mm headers are through-hole: JLCPCB fits them only with THT assembly, otherwise solder them by hand.
- **Check every part's rotation** in JLCPCB's placement preview: the CPL uses KiCad's rotation convention, which
  differs from JLCPCB's for some packages.
- Stock and prices in [bom.md](bom.md) are from a catalogue snapshot (2026-10-23): check stock when ordering.

## Before ordering v1.0

- Re-route the USB pairs as coupled pairs (today the P/N length mismatch is 5–9 mm on five of the six pairs).
- Clean up the silkscreen (labels over pads, missing connector and button labels). The header pins are labelled
  on the bottom silkscreen (`hardware/gen/add_header_labels.py`).
- Confirm the 90 Ω impedance with JLCPCB.
- Regenerate the files (`gen_fab.py`) and the [images](images/) (`gen_renders.py`) after any board change.
