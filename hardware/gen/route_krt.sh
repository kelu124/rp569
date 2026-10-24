#!/usr/bin/env bash
# Autoroute a copy of hardware/rp569.kicad_pcb with KiCadRoutingTools (docs/layout.md, Routing options B).
#   hardware/gen/route_krt.sh <workdir>     -> <workdir>/k5.kicad_pcb, DRC against the real project rules
# Needs the KRT clone (KRT, default ~/tools/KiCadRoutingTools) with its .venv and prebuilt grid_router.
# Chain (KRT's plan-pcb-routing order): GND pour on In1 -> QFN fanout U7 (RP2350B), U9 (CH569W) -> decap cleanup
# -> USB differential pairs on F.Cu -> everything else on F.Cu/In2.Cu/B.Cu.
# Flags that matter on this board (see .claude/skills/kicad-board):
#   --escalation off --fab-tier standard  no silent drop below JLCPCB standard (0.089 mm tracks, 0.25 mm vias)
#   --no-fix-drc-settings                 keep the project's net-class clearances (steps otherwise lower them)
#   --layers F.Cu In2.Cu B.Cu             In1 stays a solid GND plane (USB reference)
#   no --clearance on place_fanout_clearance: it would rewrite every class clearance to that value
set -e
W=$(realpath -m "${1:?usage: route_krt.sh <workdir>}")
HW=$(cd "$(dirname "$0")/.." && pwd)
KRT=${KRT:-$HOME/tools/KiCadRoutingTools}
P=$KRT/.venv/bin/python
F="--escalation off --fab-tier standard --no-fix-drc-settings"
POWER=(--power-nets "/GND" "/+3V3" "/3V3_CH" "/1V2_CH" "/1V1" "/VBUS_*" "/5V_*" "/*_SW" "/VREG_LX"
       --power-nets-widths 0.2 0.2 0.2 0.2 0.2 0.3 0.3 0.3 0.3)
mkdir -p "$W" && cd "$W"
cp "$HW/rp569.kicad_pcb" k0.kicad_pcb && cp "$HW/rp569.kicad_pro" k0.kicad_pro
step() { echo "$(date +%T) $1"; }
step planes;  $P $KRT/py_router/route_planes.py k0.kicad_pcb k1.kicad_pcb --nets /GND --plane-layers In1.Cu > k1.log 2>&1
step fanout;  $P $KRT/py_router/qfn_fanout.py k1.kicad_pcb -o k2a.kicad_pcb -c U7 --width 0.15 --nets "*" "!/GND" $F > k2a.log 2>&1
              $P $KRT/py_router/qfn_fanout.py k2a.kicad_pcb -o k2.kicad_pcb -c U9 --width 0.15 --nets "*" "!/GND" $F > k2.log 2>&1
step decaps;  $P $KRT/py_placer/place_fanout_clearance.py k2.kicad_pcb k3.kicad_pcb > k3.log 2>&1
step diff;    $P $KRT/py_router/route_diff.py k3.kicad_pcb k4.kicad_pcb --nets "*USB3_SS*" "*CH_SSTX*" "/USB3_D?" "/USBC_D?" \
                 "/RP_USB_D?" --track-width 0.24 --diff-pair-gap 0.13 --layers F.Cu $F > k4.log 2>&1
step route;   $P $KRT/py_router/route.py k4.kicad_pcb k5.kicad_pcb --nets "*" --layers F.Cu In2.Cu B.Cu "${POWER[@]}" $F > k5.log 2>&1
step drc      # graded against the real project rules, not the steps' own .kicad_pro
mkdir -p check && cp k5.kicad_pcb check/rp569.kicad_pcb && cp "$HW/rp569.kicad_pro" check/rp569.kicad_pro
kicad-cli pcb drc --refill-zones --format json -o check/drc.json check/rp569.kicad_pcb 2>&1 | grep -v assert | tail -1
python3 - check/drc.json <<'EOF'
import collections, json, sys
d = json.load(open(sys.argv[1]))
print(len(d['unconnected_items']), 'unconnected;', collections.Counter(v['type'] for v in d['violations']).most_common())
EOF
