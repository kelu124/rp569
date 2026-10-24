---
name: jlcpcb-parts
description: Use when choosing or checking a part for the rp569 board (LCSC number, JLCPCB stock, Basic/Extended, substitutes), or when updating the JLCPCB BOM/CPL.
---

# JLCPCB parts (rp569)

The LCSC search API and most manufacturer sites (Amphenol, Hirose) refuse scripts (HTTP 403). Use the
jlcparts snapshot of the JLCPCB catalogue through `jlc.py` (this folder):

```bash
python3 .claude/skills/jlcpcb-parts/jlc.py "0402 4.7kω ±1% chip resistor" "tps2121"
python3 .claude/skills/jlcpcb-parts/jlc.py --info C25744 C485916
```

- Search terms all have to match the lower-case index text: use `ω` for ohms, `±1%`, the package (`0402`,
  `sot-23-5`, `smd3225-4p`), the MPN. A bare value like `0ω` also matches `100ω`: check the result.
- Results: Basic parts first, then by stock. Prefer Basic (no loading fee), then big stock. Reject parts with
  stock below ~10× the board quantity (FTC303015D2R2MBCA had 9 → replaced).
- Datasheets: the `datasheet` URL from `info()` (jlcpcb.com/api/file/...) downloads with curl; TI datasheets
  from `https://www.ti.com/lit/ds/symlink/<part>.pdf`. Put them in `datasheet/<PART>/` with a markitdown `.md`.

## Where part numbers live

1. `hardware/gen/board.py`, table `LCSC_BY_VALUE`: `(lib_id, value)` → `(LCSC, MPN)`; `(lib_id, None)` is the
   fallback for every value of that symbol. `''` as LCSC = not stocked → consigned part.
2. `gen_sch.py` / `gen_pcb.py` copy them into `LCSC` and `MPN` fields (needed for schematic parity).
3. `gen_bom.py` (run by `build.sh`) writes `hardware/jlcpcb/rp569-bom.csv` and `rp569-cpl.csv`.
4. Update `docs/bom.md` (table with class/stock/price) and the specification (D-014 and the part's decision).

## Checks when swapping a part

- Footprint: use the exact KiCad footprint for the part if one exists (e.g. `LED_LiteOn_LTST-C19HE1WT`);
  otherwise generate one in `gen/mk_fp.py` from the datasheet's recommended land pattern, never from memory.
- Pin numbering: the symbol's pin numbers must match the footprint pads (RGB LED: `Device:LED_RGBA`, not
  `LED_ARGB`, for a part whose pin 4 is the anode).
- Electrical: forward voltage vs rail (3.3 V blue/green LEDs need ~100 Ω), load capacitance of crystals,
  current ratings. Record the reasoning in `specification.md`.
