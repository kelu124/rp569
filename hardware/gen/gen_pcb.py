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
W, H = 100.0, 70.0          # board size, mm
mm = pcbnew.FromMM


def P(x, y):
    return pcbnew.VECTOR2I(mm(x), mm(y))


# Fixed positions: ref-by-predicate -> (x, y, rot). Connectors on edges, ICs in their zones.
def first(pred):
    return next(p for p in board.parts if pred(p))


fixed = {
    first(lambda p: 'USB_C' in p['lib_id'])['ref']: (4.5, 35, 270),
    first(lambda p: 'USB3_B_Micro' in p['lib_id'])['ref']: (W - 4.0, 35, 90),
    first(lambda p: 'Micro_SD' in p['lib_id'])['ref']: (W - 20, H - 10, 0),
    first(lambda p: p['value'] == 'RP2350B')['ref']: (32, 35, 0),
    first(lambda p: p['value'] == 'CH569W')['ref']: (68, 32, 0),
    first(lambda p: p['value'] == 'LA IN')['ref']: (9, 4, 90),
    first(lambda p: p['value'] == 'RP EXT')['ref']: (36, 4, 90),
    first(lambda p: p['value'] == 'CH EXT')['ref']: (41, H - 6, 90),
    first(lambda p: p['value'] == 'SWD')['ref']: (62.5, 4, 90),
    first(lambda p: p['value'] == 'RP I2C')['ref']: (71.5, 4, 90),
    first(lambda p: p['value'] == 'RP UART')['ref']: (84.5, 4, 90),
    first(lambda p: p['value'] == 'CH I2C')['ref']: (17, H - 4, 90),
    first(lambda p: p['value'] == 'CH UART')['ref']: (29.5, H - 4, 90),
}
holes = [p['ref'] for p in board.parts if p['lib_id'] == 'Mechanical:MountingHole']
for ref, xy in zip(holes, [(3.5, 3.5), (W - 3.5, 3.5), (3.5, H - 3.5), (W - 3.5, H - 3.5)]):
    fixed[ref] = (*xy, 0)

# Placement zones (x0, y0, x1, y1) for the remaining parts
ZONES = {
    'pwr_rp': (9, 44, 24, 62),
    'pwr_ch': (78, 42, 96, 56),
    'RP2350B': (14, 10, 46, 60),
    'Logic analyser': (4, 8, 14, 20),
    'Inter-chip': (44, 22, 56, 46),
    'CH569W': (58, 10, 90, 46),
    'USB3 / SD': (78, 12, 92, 30),
}


def zone_of(p):
    if p['block'] != 'Power':
        return p['block']
    nets = set(p['pins'].values())
    ch_nets = {'VBUS_3', '5V_CH_IN', '5V_CH', '3V3_CH', '1V2_CH', '3V3_CH_SW', '1V2_CH_SW', '3V3_CH_FB',
               '1V2_CH_FB', 'CH569_PWR_EN', 'CH569_PWR_FAULT_N', 'VBUS3_SENSE', 'CH3V3_SENSE'}
    return 'pwr_ch' if nets & ch_nets and 'VBUS_C' not in nets else 'pwr_rp'


def load_fp(fpid):
    lib, name = fpid.split(':')
    fp = pcbnew.FootprintLoad(FPLIB + lib + '.pretty', name)
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

    # Nearest-free-spot placement: each part goes as close as possible to the pads it connects to.
    POWER = {'GND', '+3V3', '1V1', '3V3_CH', '1V2_CH', '5V_SYS', '5V_CH'}
    margin = 1.0
    occupied = [fps[r][0].GetBoundingBox(False) for r in fixed]
    placed = set(fixed)
    pad_pos = {}

    def refresh(ref):
        fp = fps[ref][0]
        for pad in fp.Pads():
            n = pad.GetNetname()
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
        pts = [pos for n in (sig or nets) for (_, pos, _) in pad_pos.get(n, []) if n != 'GND']
        if pts:
            return pcbnew.VECTOR2I(sum(q.x for q in pts) // len(pts), sum(q.y for q in pts) // len(pts))
        x0, y0, x1, y1 = ZONES.get(zone_of(p), (10, 10, W - 10, H - 10))
        return P((x0 + x1) / 2, (y0 + y1) / 2)

    offsets = sorted(((dx * 0.5, dy * 0.5) for dx in range(-70, 71) for dy in range(-70, 71)),
                     key=lambda t: t[0] ** 2 + t[1] ** 2)
    # order: big parts first, then parts touching already-placed nets
    rest = [r for r in fps if r not in placed]
    rest.sort(key=lambda r: -fps[r][0].GetBoundingBox(False).GetArea())
    for ref in rest:
        fp, p = fps[ref]
        t = target(fp, p)
        tx, ty = pcbnew.ToMM(t.x), pcbnew.ToMM(t.y)
        fp.SetPosition(P(0, 0))
        bb = fp.GetBoundingBox(False)
        bx, by = pcbnew.ToMM(bb.GetX()), pcbnew.ToMM(bb.GetY())
        bw, bh = pcbnew.ToMM(bb.GetWidth()), pcbnew.ToMM(bb.GetHeight())
        ok = False
        for dx, dy in offsets:
            x, y = tx + dx, ty + dy
            if x + bx < margin or y + by < margin or x + bx + bw > W - margin or y + by + bh > H - margin:
                continue
            fp.SetPosition(P(x, y))
            nb = fp.GetBoundingBox(False)
            nb.Inflate(mm(0.25))
            if any(nb.Intersects(o) for o in occupied):
                continue
            occupied.append(nb)
            ok = True
            break
        if not ok:
            print(f'WARNING: no room for {ref}')
        placed.add(ref)
        refresh(ref)

    # outline
    seg = [(0, 0, W, 0), (W, 0, W, H), (W, H, 0, H), (0, H, 0, 0)]
    for x0, y0, x1, y1 in seg:
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
        for x, y in ((0.5, 0.5), (W - 0.5, 0.5), (W - 0.5, H - 0.5), (0.5, H - 0.5)):
            ol.Append(mm(x), mm(y))
        brd.Add(z)
    pcbnew.ZONE_FILLER(brd).Fill(brd.Zones())

    out = os.path.join(HW, 'rp569.kicad_pcb')
    brd.Save(out)
    print(f'{len(fps)} footprints, {len(nets)} nets -> {out}')


if __name__ == '__main__':
    main()
