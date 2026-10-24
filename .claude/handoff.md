# Handoff — where the rp569 work stands (2026-10-24)

Read this first when resuming. Details live in `specification.md` (D-xxx), `docs/layout.md`, `.claude/todo.md`.

## State

- **v1.0 board done**: `hardware/rp569-routed.kicad_pcb` (+ `rp569-routed.kicad_pro`), tag `v1.0`. Fully routed:
  0 unconnected, 0 DRC errors (119 warnings: 116 silkscreen, 2 lib_footprint, 1 VREG_AVDD stub that is part of
  the connection). 3 mm rounded corners; header pin labels on B.SilkS; SW1 printed `RPDFU`, SW5 `BOOT` (D-023).
- Board post-processing scripts, both idempotent, both edit the routed board in place (close it in KiCad first):
  `/usr/bin/python3 hardware/gen/add_header_labels.py` (silkscreen labels, group `header_labels`) and
  `/usr/bin/python3 hardware/gen/round_corners.py`. After any board change: `gen_fab.py`, then `gen_renders.py`.
- **Production files**: `hardware/jlcpcb/rp569-gerbers.zip`, `rp569-bom.csv`, `rp569-cpl.csv` from
  `python3 hardware/gen/gen_fab.py` (DRC-gated). 2 consigned parts: CH569W, GSB343K33HR.
- Images: `python3 hardware/gen/gen_renders.py` → `docs/images/` (re-run after board changes).
- Docs site: `docs/` is the GitHub Pages root (`index.md`, `_config.yml`); enable it in the repo settings
  (Pages → main → /docs) if not done. URL https://kelu124.github.io/rp569/.
- `hardware/rp569.kicad_pcb` is the **generated, unrouted** board (`build.sh`); it is not the one to fabricate.
- How v1.0 was made: generator (`board.py` → `build.sh`) → KiCadRoutingTools chain (`hardware/gen/route_krt.sh`,
  16 links left) → hand finishing in pcbnew by the user, checked with `.claude/skills/kicad-board/drc_check.py`.

## Next steps (before ordering)

1. USB pairs as coupled pairs (skew 5–9 mm today on USB3_SSTX/SSRX, USB3_D, USBC_D, RP_USB_D; CH_SSTX is fine):
   delete both lines, pcbnew router **6** on F.Cu, then *Tune skew*. Check with `board_metrics.py`.
2. Silkscreen: ~68 reference labels over copper; connector names and the other buttons (SW2–4, SW6–7) still show
   references only. Header pin labels and RPDFU/BOOT are done (`add_header_labels.py`, re-run after moving a
   header; add names to its `BUTTON_LABELS`).
3. Confirm 0.24/0.13 mm = 90 Ω with JLCPCB's impedance calculator (JLC04161H-7628).
4. Re-run `python3 hardware/gen/gen_fab.py`, then review gerbers and the JLCPCB placement preview (rotations).
5. Later: HSPI lengths 19.9–32.6 mm vs RCLK 20.6 (fine below ~100 MHz); open firmware questions in todo.

## Tools on this machine

| Tool | Where | Notes |
|---|---|---|
| KiCad 10.0.6 | `kicad-cli`, `/usr/bin/python3` has `pcbnew` | local help: `/usr/share/doc/kicad/help/en/pcbnew.html` |
| KiCadRoutingTools v0.23.7 | `~/tools/KiCadRoutingTools` (`.venv`, prebuilt Rust router) | always `--escalation off --fab-tier standard --no-fix-drc-settings`, `--layers F.Cu In2.Cu B.Cu`; no `--clearance` on the decap step |
| Freerouting 2.1.0 patched | `~/.local/share/freerouting/freerouting-2.1.0-mp.jar` (Java 21) | option only; `hardware/gen/patch_freerouting_mp.py` rebuilds it |
| jlcparts lookup | `.claude/skills/jlcpcb-parts/jlc.py` | LCSC API is blocked; cache `~/.cache/jlcparts` |

## Working agreements with the user

- Commit and push to `main` when asked (usually after each step); log in `.claude/commits.md` (pending → hash).
- Watch every autorouter run with a 3-minute Monitor report (skill `kicad-board`).
- Record decisions in `specification.md`, learnings in skills and memory.
- Never write `rp569-routed.kicad_pcb` while KiCad has it open; refill zones (B) before pcbnew's Cleanup.
