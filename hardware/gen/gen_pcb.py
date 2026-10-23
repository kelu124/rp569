"""Generate hardware/rp569.kicad_pcb from board.py: footprints, nets, block placement, outline, zones.

Run with the system Python that ships pcbnew:  /usr/bin/python3 gen_pcb.py
Routing is NOT done here (see docs/layout.md).
"""
import os
import sys

import pcbnew

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import board  # noqa: E402
from gen_sch import libfile, uid  # noqa: E402
from kisym import get_symbol, pins  # noqa: E402

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FPLIB = '/usr/share/kicad/footprints/'
W, H = 50.0, 70.0           # board size, mm
CHAMFER = 3.0               # 45-degree corner chamfer, mm
mm = pcbnew.FromMM


def P(x, y):
    return pcbnew.VECTOR2I(mm(x), mm(y))


# Fixed positions: ref-by-predicate -> (x, y, rot). Connectors on edges, ICs in their zones.
def first(pred):
    return next(p for p in board.parts if pred(p))


def by_value(v):
    return first(lambda p: p['value'] == v)['ref']


# Top side: both chips, USB connectors, microSD, LEDs, buttons. RP2350 in the top half (USB-C on the
# top edge), CH569 in the bottom half (USB3 + microSD on the bottom edge), HSPI bus in between.
fixed = {
    first(lambda p: 'USB_C' in p['lib_id'])['ref']: (25, 4.3, 180),
    first(lambda p: 'USB3_B_Micro' in p['lib_id'])['ref']: (16, H - 1.45, 0),   # slots 1.45 mm from the edge
    first(lambda p: 'Micro_SD' in p['lib_id'])['ref']: (33.5, H - 9.0, 0),
    by_value('RP2350B'): (25, 21, 0),
    by_value('CH569W'): (25, 45, 0),
    # headers (THT, both sides): along the edges
    by_value('SWD'): (10, 2.2, 90),
    by_value('RP I2C'): (33, 2.2, 90),
    by_value('LA IN'): (2.2, 10.5, 0),
    by_value('RP EXT'): (W - 2.2, 9.5, 0),
    by_value('CH EXT'): (2.2, 38, 0),
    by_value('CH I2C'): (W - 2.2, 37, 0),
    by_value('CH UART'): (W - 2.2, 48.3, 0),
    by_value('CH BOOT JP'): (W - 2.2, 57.2, 0),
    # buttons and RGB LEDs, each next to its chip
    by_value('BOOTSEL'): (10.5, 8, 0),
    by_value('RUN'): (10.5, 12.5, 0),
    by_value('RP BTN1'): (39.5, 8, 0),
    by_value('RP BTN2'): (39.5, 12.5, 0),
    by_value('CH BTN1'): (10.5, 54.5, 0),
    by_value('CH BTN2'): (16.7, 54.5, 0),
    by_value('CH569 BOOT'): (13.5, 59, 0),
    first(lambda p: p['lib_id'] == 'Device:LED_ARGB' and p['block'] == 'RP2350B')['ref']: (34, 13, 0),
    first(lambda p: p['lib_id'] == 'Device:LED_ARGB' and p['block'] == 'CH569W')['ref']: (37.5, 41, 0),
}
holes = [p['ref'] for p in board.parts if p['lib_id'] == 'Mechanical:MountingHole']
for ref, xy in zip(holes, [(4, 4), (W - 4, 4), (4, H - 4), (W - 4, H - 4)]):
    fixed[ref] = (*xy, 0)

# Fallback zones (x0, y0, x1, y1) for parts with no placed neighbour
ZONES = {
    'pwr_rp': (6, 26, 20, 36),
    'pwr_ch': (30, 50, 45, 60),
    'RP2350B': (8, 14, 42, 30),
    'Logic analyser': (5, 12, 12, 30),
    'Inter-chip': (12, 30, 38, 38),
    'CH569W': (8, 38, 42, 52),
    'USB3 / SD': (20, 52, 42, 62),
}
# parts that must stay on the top side, next to the connectors (no via on the high-speed pairs)
TOP_ONLY_NETS = {'USB3_SSTX_P', 'USB3_SSTX_N', 'USB3_SSRX_P', 'USB3_SSRX_N', 'CH_SSTX_P', 'CH_SSTX_N',
                 'USB3_DP', 'USB3_DM', 'USBC_DP', 'USBC_DM'}


def zone_of(p):
    if p['block'] != 'Power':
        return p['block']
    nets = set(p['pins'].values())
    ch_nets = {'VBUS_3', '5V_CH_IN', '5V_CH', '3V3_CH', '1V2_CH', '3V3_CH_SW', '1V2_CH_SW', '3V3_CH_FB',
               '1V2_CH_FB', 'CH569_PWR_EN', 'CH569_PWR_FAULT_N', 'VBUS3_SENSE', 'CH3V3_SENSE'}
    return 'pwr_ch' if nets & ch_nets and 'VBUS_C' not in nets else 'pwr_rp'


def load_fp(fpid):
    lib, name = fpid.split(':')
    path = os.path.join(HW, 'lib', 'rp569.pretty') if lib == 'rp569' else FPLIB + lib + '.pretty'
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None:
        raise SystemExit(f'footprint not found: {fpid}')
    return fp


def main():
    brd = pcbnew.BOARD()
    brd.SetCopperLayerCount(4)
    ds = brd.GetDesignSettings()
    ds.m_TrackMinWidth = mm(0.127)
    ds.m_MinClearance = mm(0.127)
    ds.m_ViasMinSize = mm(0.45)
    ds.m_MinThroughDrill = mm(0.2)

    nets = {}
    for p in board.parts:
        for n in p['pins'].values():
            if n not in nets:
                ni = pcbnew.NETINFO_ITEM(brd, '/' + n)   # same names as the schematic (root-sheet labels)
                brd.Add(ni)
                nets[n] = ni

    fps = {}
    for p in board.parts:
        if p['ref'].startswith('#'):
            continue
        fp = load_fp(p['fp'])
        fp.SetReference(p['ref'])
        fp.SetValue(p['value'])
        fp.SetPath(pcbnew.KIID_PATH('/' + uid('sym', p['ref'])))
        fp.SetFPIDAsString(p['fp'])
        if p['dnp']:
            fp.SetDNP(True)
        names = {num: name for num, name, *_ in pins(get_symbol(libfile(p['lib_id']), p['lib_id'].split(':')[1]))}
        for pad in fp.Pads():
            num = pad.GetNumber()
            n = p['pins'].get(num)
            if n:
                pad.SetNet(nets[n])
            elif num in names:      # same auto-name eeschema gives a no-connect pin
                nn = f'unconnected-({p["ref"]}-{names[num]}-Pad{num})'
                ni = pcbnew.NETINFO_ITEM(brd, nn)
                brd.Add(ni)
                pad.SetNet(ni)
        brd.Add(fp)
        fps[p['ref']] = (fp, p)

    # fixed parts
    for ref, (x, y, rot) in fixed.items():
        fp = fps[ref][0]
        fp.SetOrientationDegrees(rot)
        fp.SetPosition(P(x, y))
        if ref in holes:
            fp.Reference().SetVisible(False)

    # Nearest-free-spot placement on two sides: each part goes as close as possible to the pads it connects
    # to. Small passives prefer the bottom side (decoupling right under the IC pads), other parts the top.
    POWER = {'GND', '+3V3', '1V1', '3V3_CH', '1V2_CH', '5V_SYS', '5V_CH'}
    margin = 1.0
    occupied = {'F': [], 'B': []}

    def is_tht(fp):
        return any(pad.GetAttribute() in (pcbnew.PAD_ATTRIB_PTH, pcbnew.PAD_ATTRIB_NPTH) for pad in fp.Pads())

    for r in fixed:
        fp = fps[r][0]
        occupied['F'].append(fp.GetBoundingBox(False))
        if is_tht(fp):
            occupied['B'].append(fp.GetBoundingBox(False))
        elif fps[r][1]['value'] in ('RP2350B', 'CH569W'):    # keep the bottom free under the EP (thermal vias)
            ep = max(fp.Pads(), key=lambda pad: pad.GetBoundingBox().GetArea()).GetBoundingBox()
            ep.Inflate(mm(0.5))
            occupied['B'].append(ep)
    placed = set(fixed)
    pad_pos = {}

    def refresh(ref):
        fp = fps[ref][0]
        for pad in fp.Pads():
            n = pad.GetNetname().lstrip('/')      # board nets carry the schematic's '/' prefix
            if n:
                pad_pos.setdefault(n, []).append((ref, pad.GetPosition(), pad.GetNumber()))

    for r in fixed:
        refresh(r)
    # decoupling: assign each IC-block cap whose nets are {rail, GND} to one IC power pad of that rail
    claimed = set()

    def target(fp, p):
        nets = set(p['pins'].values())
        sig = nets - POWER
        if p['block'] == 'Power':      # power-path parts follow the power nets, not their control signals
            sig = (nets & {'VBUS_C', 'VBUS_3', '5V_SYS', '5V_CH_IN', '5V_CH', '3V3_CH_SW', '1V2_CH_SW', '+3V3_SW'}) or sig
        if p['lib_id'] == 'Device:C' and not sig:
            rail = (nets - {'GND'}) or nets
            rail = next(iter(rail))
            for ref, pos, num in pad_pos.get(rail, []):
                ic = fps[ref][1]
                if ic['ref'].startswith('U') and ic['block'] == p['block'] and (ref, num) not in claimed:
                    claimed.add((ref, num))
                    return pos
        pts = [pos for n in (sig or nets) for (_, pos, _) in pad_pos.get(n, [])
               if n != 'GND']
        if pts:
            return pcbnew.VECTOR2I(sum(q.x for q in pts) // len(pts), sum(q.y for q in pts) // len(pts))
        x0, y0, x1, y1 = ZONES.get(zone_of(p), (10, 10, W - 10, H - 10))
        return P((x0 + x1) / 2, (y0 + y1) / 2)

    offsets = sorted(((dx * 0.5, dy * 0.5) for dx in range(-70, 71) for dy in range(-70, 71)),
                     key=lambda t: t[0] ** 2 + t[1] ** 2)

    def find_spot(fp, side, tx, ty):
        fp.SetPosition(P(0, 0))
        bb = fp.GetBoundingBox(False)
        bx, by = pcbnew.ToMM(bb.GetX()), pcbnew.ToMM(bb.GetY())
        bw, bh = pcbnew.ToMM(bb.GetWidth()), pcbnew.ToMM(bb.GetHeight())
        for dx, dy in offsets:
            x, y = tx + dx, ty + dy
            if x + bx < margin or y + by < margin or x + bx + bw > W - margin or y + by + bh > H - margin:
                continue
            fp.SetPosition(P(x, y))
            nb = fp.GetBoundingBox(False)
            nb.Inflate(mm(0.25))
            if any(nb.Intersects(o) for o in occupied[side]):
                continue
            return (dx * dx + dy * dy) ** 0.5, x, y, nb
        return None

    PASSIVE = {'Device:R', 'Device:C', 'Device:R_Pack04'}
    # order: big parts first
    rest = [r for r in fps if r not in placed]
    rest.sort(key=lambda r: -fps[r][0].GetBoundingBox(False).GetArea())
    for ref in rest:
        fp, p = fps[ref]
        t = target(fp, p)
        tx, ty = pcbnew.ToMM(t.x), pcbnew.ToMM(t.y)
        top_only = bool(set(p['pins'].values()) & TOP_ONLY_NETS)
        bias = {'F': 2.0, 'B': 0.0} if p['lib_id'] in PASSIVE else {'F': 0.0, 'B': 4.0}
        cand = {'F': find_spot(fp, 'F', tx, ty)}
        if not top_only:
            fp.SetPosition(P(0, 0))
            fp.SetLayerAndFlip(pcbnew.B_Cu)
            cand['B'] = find_spot(fp, 'B', tx, ty)
        cand = {k: v for k, v in cand.items() if v}
        if not cand:
            print(f'WARNING: no room for {ref}')
            continue
        side = min(cand, key=lambda k: cand[k][0] + bias[k])
        _, x, y, nb = cand[side]
        if side == 'F' and fp.IsFlipped():
            fp.SetPosition(P(0, 0))
            fp.SetLayerAndFlip(pcbnew.F_Cu)
        fp.SetPosition(P(x, y))
        occupied[side].append(nb)
        if is_tht(fp):
            occupied['B' if side == 'F' else 'F'].append(nb)
        placed.add(ref)
        refresh(ref)

    # outline
    c = CHAMFER
    corners = [(c, 0), (W - c, 0), (W, c), (W, H - c), (W - c, H), (c, H), (0, H - c), (0, c)]
    for (x0, y0), (x1, y1) in zip(corners, corners[1:] + corners[:1]):
        s = pcbnew.PCB_SHAPE(brd)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(P(x0, y0))
        s.SetEnd(P(x1, y1))
        s.SetLayer(pcbnew.Edge_Cuts)
        s.SetWidth(mm(0.1))
        brd.Add(s)

    # GND planes on In1 and In2 (reference planes for USB3 / HSPI)
    for layer in (pcbnew.In1_Cu, pcbnew.In2_Cu):
        z = pcbnew.ZONE(brd)
        z.SetLayer(layer)
        z.SetNet(nets['GND'])
        z.SetLocalClearance(mm(0.2))
        z.SetMinThickness(mm(0.2))
        ol = z.Outline()
        ol.NewOutline()
        for x, y in corners:      # board outline pulled in by 0.5 mm
            ol.Append(mm(x + (0.5 if x < W / 2 else -0.5)), mm(y + (0.5 if y < H / 2 else -0.5)))
        brd.Add(z)
    pcbnew.ZONE_FILLER(brd).Fill(brd.Zones())

    out = os.path.join(HW, 'rp569.kicad_pcb')
    brd.Save(out)
    print(f'{len(fps)} footprints, {len(nets)} nets -> {out}')


if __name__ == '__main__':
    main()
