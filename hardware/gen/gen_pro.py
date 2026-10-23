"""Write hardware/rp569.kicad_pro with net classes (values are placeholders until the stackup is fixed)."""
import json
import os

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def nc(name, track, clearance=0.15, dp_width=None, dp_gap=None, prio=0):
    d = {'name': name, 'track_width': track, 'clearance': clearance, 'via_diameter': 0.45, 'via_drill': 0.2,
         'microvia_diameter': 0.3, 'microvia_drill': 0.1, 'bus_width': 12, 'wire_width': 6, 'line_style': 0,
         'pcb_color': 'rgba(0, 0, 0, 0.000)', 'schematic_color': 'rgba(0, 0, 0, 0.000)', 'priority': prio,
         'tuning_profile': ''}
    if dp_width:
        d.update(diff_pair_width=dp_width, diff_pair_gap=dp_gap, diff_pair_via_gap=0.25)
    return d


classes = [
    nc('Default', 0.15, clearance=0.1, prio=2147483647),
    # 90 ohm differential: width/gap must be recalculated for the fab's 4-layer stackup (see docs/layout.md)
    nc('USB3_90R', 0.15, dp_width=0.15, dp_gap=0.15, prio=0),
    nc('USB2_90R', 0.15, dp_width=0.15, dp_gap=0.15, prio=1),
    nc('HSPI', 0.15, prio=2),
    nc('Power', 0.4, prio=3),
]
patterns = [
    ('USB3_90R', '/USB3_SS*'), ('USB3_90R', '/CH_SSTX_*'),
    ('USB2_90R', '/USB3_D?'), ('USB2_90R', '/USBC_D?'), ('USB2_90R', '/RP_USB_D?'),
    ('HSPI', '/HSPI_*'),
    ('Power', '/VBUS_*'), ('Power', '/5V_*'), ('Power', '/+3V3*'), ('Power', '/3V3_CH*'), ('Power', '/1V2_CH*'),
    ('Power', '/1V1'), ('Power', '/VREG_LX'), ('Power', '/GND'),
]
path = os.path.join(HW, 'rp569.kicad_pro')
# merge into the full project file KiCad writes (a minimal file gets rewritten with defaults by kicad-cli)
pro = json.load(open(path)) if os.path.exists(path) else {'meta': {'filename': 'rp569.kicad_pro', 'version': 3}}
ns = pro.setdefault('net_settings', {})
default = next((c for c in ns.get('classes', []) if c['name'] == 'Default'), {})
ns['classes'] = [{**default, **c} if c['name'] == 'Default' else {**{k: v for k, v in default.items() if k != 'name'}, **c}
                 for c in classes]
ns['netclass_patterns'] = [{'netclass': c, 'pattern': p} for c, p in patterns]
with open(path, 'w') as f:
    json.dump(pro, f, indent=2)
