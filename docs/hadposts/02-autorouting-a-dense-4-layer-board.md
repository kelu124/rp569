---
title: "Autorouting a dense 4-layer board: from 421 open links to zero"
status: draft
target: hackaday.io project log
images: ../images/rp569-v1.0-top.png (and ideally a before/after ratsnest screenshot)
---

# Autorouting a dense 4-layer board: from 421 open links to zero

rp569 is 50 × 70 mm, four layers, ~150 parts on both sides, two 0.4 mm-pitch QFNs (RP2350B QFN-80 and CH569 QFN-68),
USB 3 and USB 2 differential pairs and a 16-line bus between the chips. I wanted the routing to be reproducible from
scripts as far as possible, so this is the log of getting an autorouter to do most of it, and what it taught me
about the *placement*.

Layer plan: F.Cu, In2.Cu and B.Cu for routing; In1.Cu a solid GND plane, the reference for the 90 Ω USB pairs.

## Round 1: Freerouting, and why it stalled

Freerouting (2.1.0, the last release that runs on Java 21) got stuck at 170–240 unrouted links out of 421 and
stayed there. Most of the reasons were not congestion:

1. **The net classes never reached it.** Saving a generated board rewrote the `.kicad_pro` next to it with KiCad's
   defaults, so the exported DSN said 0.2 mm track / 0.2 mm clearance for every net. Around 0.4 mm-pitch QFN pads
   that is close to unroutable. Lesson: always `grep "(width\|(clearance"` the DSN before trusting a result.
2. **`-mp` (max passes) is ignored in 2.1.0.** The CLI stores it, but the stop check reads another field the CLI
   never sets, so runs went on to the default 12-hour job timeout. Worse, a job timeout stops it mid-pass, with the
   connections it was re-routing ripped up, so the saved result is worse than the last completed pass. I patched
   one instruction in `BatchAutorouter.class` (a `getfield maxPasses` instead of `invokevirtual get_stop_pass_no`,
   same length, same stack effect) and `-mp` works.
3. **No vias to the GND plane.** With In1 declared as a plane, Freerouting places zero vias for GND: GND pads only
   connected by luck through outer-layer pours. A small script now drops a via next to every SMD GND pad before
   export.
4. **0.3 mm power tracks can't enter 0.2 mm QFN pins**, and Freerouting does not neck down. A separate 0.2 mm
   "Rail" class for the logic rails fixed most of the stuck rail pins.

With all that: 35 unrouted after 40 passes, and the count oscillated instead of converging.

## Round 2: KiCadRoutingTools

[KiCadRoutingTools](https://github.com/drandyhaas/KiCadRoutingTools) is a Rust A* router with rip-up and reroute,
real coupled differential pairs, QFN fan-out, neck-down at fine-pitch pads and plane handling. Its chain is:
pour the planes, fan out the QFNs, route the diff pairs, route the rest. A full chain takes ~10 minutes.

Three defaults had to be overridden for this board:

- `--escalation off --fab-tier standard`: otherwise it quietly relaxes the fab rules (0.089 mm tracks, 0.25 mm
  vias, hole clearance 0.1 mm) when it gets stuck.
- `--layers F.Cu In2.Cu B.Cu`: otherwise it happily routes 233 segments on the GND plane.
- `--no-fix-drc-settings`, and no `--clearance` on the decap-cleanup step: one step rewrote every net-class
  clearance to 0.1 mm in its output project, and the next steps inherited it. The "13 unrouted" result I first
  celebrated had 172 clearance violations against my real rules.

## The placement mattered more than the router

Every big improvement came from changing the board, not the router settings:

| Change | Unrouted after routing |
|---|---|
| First Freerouting run | ~200 of 421 |
| Rail class, GND fan-out vias, Freerouting fixed | 35 of 405 |
| KiCadRoutingTools, QFN fan-out, original clearances | 32 |
| HSPI and power clearance 0.15 → 0.1 mm (USB kept at 0.13) | 27 |
| + a second route pass | 21 |
| TPS2121, sense dividers and pull-ups moved out of the corridor between the chips | 16 |
| Hand finishing in pcbnew | **0** |

The placement fixes, in order of effect:

- **Assign pins by chip side.** On both chips, every reassignable GPIO (UARTs and I2C on the RP2350, plain GPIO on
  the CH569) was picked so the line leaves the package on the side facing its destination. UARTs and I2C buses
  stayed on valid, separate peripheral instances (checked against the RP2350 GPIO function table).
- **Drop what you don't need.** Sixteen 33 Ω series resistors on 2–3 cm HSPI lines (four resistor arrays in the
  most crowded spot of the board) went away; drive strength and slew can be set in firmware instead.
- **Turn the passives.** The placer left every resistor at 0°/180°; the arrays had both pin columns facing
  sideways to the signal flow.
- **Watch out for centroid placement.** A part whose nets go to two distant places lands half-way, which is often
  the busiest spot. The power mux, fed from connectors at the top and the right edge, ended up in the middle of the
  HSPI corridor; so did the 3.3 V sense divider and the inter-chip pull-ups. Fixing the mux at its main input,
  dividers at the rail they measure and pull-ups at their chip's pin cleared the corridor.

## Hand finishing, and one trap

The last 16 links were routed by hand in pcbnew, with a DRC check after each save. Two lessons:

- Some of them only fit by crossing the GND plane layer. I accepted that for v1.0 after checking that the plane is
  still one piece and that the only gaps under the USB pairs are via antipads.
- *Edit → Cleanup Tracks & Vias* with stale zone fills deleted 22 GND vias, because their only other connection is
  the plane fill: 0 unconnected became 22. Refill zones (B) before any cleanup.

## What's left

The USB pairs still need re-routing as coupled pairs (KiCad's diff-pair router, then skew tuning), and the
silkscreen needs a pass. Then gerbers, BOM and placement go to JLCPCB.

*Made with KiCad 10, Freerouting 2.1.0, KiCadRoutingTools, and a lot of DRC runs (and Claude Code doing the
scripting and analysis).*
