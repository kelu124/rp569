#!/usr/bin/env bash
# Regenerate the KiCad project from board.py and check it (ERC, DRC + schematic parity).
# Usage: hardware/gen/build.sh   (reports go to hardware/reports/)
set -euo pipefail
cd "$(dirname "$0")"
python3 mk_lib.py
python3 mk_fp.py
python3 gen_sch.py
/usr/bin/python3 gen_pcb.py 2>&1 | grep -v 'assert "m_choices' || true   # pcbnew lives in the system Python
python3 gen_pro.py
mkdir -p ../reports
kicad-cli sch erc -o ../reports/erc.rpt ../rp569.kicad_sch | grep -v '^Saved' || true
python3 gen_pro.py   # kicad-cli may rewrite the project file; re-apply net classes
python3 gen_bom.py
kicad-cli pcb drc --schematic-parity -o ../reports/drc.rpt ../rp569.kicad_pcb | grep Found || true
grep -E '^\[' ../reports/drc.rpt | sed 's/:.*//' | sort | uniq -c
