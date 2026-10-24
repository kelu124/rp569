"""Autoroute hardware/rp569.kicad_pcb with Freerouting (Specctra DSN -> SES round trip).

Run with the system Python (pcbnew):  /usr/bin/python3 route.py [passes] [board.kicad_pcb] [--keep]
Needs Java 21 and Freerouting 2.1.0 patched so that -mp works (FREEROUTING_JAR, default
~/.local/share/freerouting/freerouting-2.1.0-mp.jar, built by patch_freerouting_mp.py).
Freerouting 2.2+ needs Java 25. It does not route differential pairs as coupled pairs: the USB pairs must be
checked, and re-routed with pcbnew's diff-pair router if needed (docs/layout.md).
"""
import math
import os
import shutil
import subprocess
import sys

import pcbnew

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_pcb import add_gnd_zone  # noqa: E402

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PCB = os.path.join(HW, 'rp569.kicad_pcb')
JAR = os.environ.get('FREEROUTING_JAR', os.path.expanduser('~/.local/share/freerouting/freerouting-2.1.0-mp.jar'))


def rect_dist(bb, x, y):
    """Distance (mm) from point x, y to a pcbnew BOX2I, 0 inside."""
    dx = max(bb.GetLeft() / 1e6 - x, 0, x - bb.GetRight() / 1e6)
    dy = max(bb.GetTop() / 1e6 - y, 0, y - bb.GetBottom() / 1e6)
    return math.hypot(dx, dy)


def gnd_fanout(brd, via_d=0.5, drill=0.3, clear=0.15, width=0.2):
    """A locked via to the In1 plane next to every SMD GND pad (Freerouting leaves plane nets unconnected), plus
    four vias inside each exposed pad. Returns the number of vias added."""
    gnd = brd.FindNet('/GND')
    pads = [p for p in brd.GetPads()]
    edge = brd.GetBoardEdgesBoundingBox()
    vias = []          # (x, y) mm of every via placed so far
    keepouts = [z for fp in brd.GetFootprints() for z in fp.Zones() if z.GetIsRuleArea()]

    def kept_out(x, y, r):
        pts = [(x, y)] + [(x + r * math.cos(k * math.pi / 4), y + r * math.sin(k * math.pi / 4)) for k in range(8)]
        return any(z.Outline().Contains(pcbnew.VECTOR2I(pcbnew.FromMM(a), pcbnew.FromMM(b)))
                   for z in keepouts for a, b in pts)

    def free(x, y, own):
        if kept_out(x, y, via_d / 2 + clear):
            return False
        if rect_dist(edge, x, y) > 0 or min(x - edge.GetLeft() / 1e6, edge.GetRight() / 1e6 - x,
                                            y - edge.GetTop() / 1e6, edge.GetBottom() / 1e6 - y) < 1.0:
            return False
        if any(math.hypot(x - vx, y - vy) < via_d + clear for vx, vy in vias):
            return False
        for q in pads:
            if q is own:
                continue
            gap = via_d / 2 + (clear if q.GetNetCode() != gnd.GetNetCode() else 0.05)
            if rect_dist(q.GetBoundingBox(), x, y) < gap:
                return False
        return True

    def add_via(x, y):
        v = pcbnew.PCB_VIA(brd)
        v.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y)))
        v.SetWidth(pcbnew.FromMM(via_d))
        v.SetDrill(pcbnew.FromMM(drill))
        v.SetNet(gnd)
        v.SetLocked(True)
        brd.Add(v)
        vias.append((x, y))

    n = 0
    for p in pads:
        if p.GetNetCode() != gnd.GetNetCode() or p.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
            continue
        px, py = pcbnew.ToMM(p.GetPosition().x), pcbnew.ToMM(p.GetPosition().y)
        sx, sy = pcbnew.ToMM(p.GetBoundingBox().GetWidth()), pcbnew.ToMM(p.GetBoundingBox().GetHeight())
        if sx > 2 and sy > 2:                       # exposed pad: vias inside it
            for ox, oy in ((-1, -1), (1, -1), (-1, 1), (1, 1)):
                add_via(px + ox * sx / 4, py + oy * sy / 4)
                n += 1
            continue
        layer = pcbnew.F_Cu if p.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu
        best = None
        for step in (0.0, 0.2, 0.4, 0.7):
            for k in range(16):
                a = k * math.pi / 8
                ux, uy = math.cos(a), math.sin(a)
                # leave the pad along (ux, uy): half-extent of its bounding box in that direction
                r = min(sx / 2 / abs(ux) if ux else 1e9, sy / 2 / abs(uy) if uy else 1e9) + via_d / 2 + 0.05 + step
                x, y = px + ux * r, py + uy * r
                if free(x, y, p) and not kept_out((px + x) / 2, (py + y) / 2, width / 2) and all(rect_dist(q.GetBoundingBox(), (px + x) / 2, (py + y) / 2) >= width / 2 + 0.1
                                         for q in pads if q is not p and q.GetNetCode() != gnd.GetNetCode()):
                    best = (x, y)
                    break
            if best:
                break
        if not best:
            continue
        add_via(*best)
        t = pcbnew.PCB_TRACK(brd)
        t.SetStart(p.GetPosition())
        t.SetEnd(pcbnew.VECTOR2I(pcbnew.FromMM(best[0]), pcbnew.FromMM(best[1])))
        t.SetWidth(pcbnew.FromMM(width))
        t.SetLayer(layer)
        t.SetNet(gnd)
        t.SetLocked(True)
        brd.Add(t)
        n += 1
    return n


def main():
    keep = '--keep' in sys.argv          # continue from the board's current routing instead of starting over
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    passes = int(args[0]) if args else 40
    pcb = os.path.abspath(args[1]) if len(args) > 1 else PCB
    work = os.path.join(os.path.dirname(pcb), 'route')
    os.makedirs(work, exist_ok=True)
    dsn, ses = os.path.join(work, 'rp569.dsn'), os.path.join(work, 'rp569.ses')
    if pcb != PCB:   # a variant board: its project file must carry the net classes (saving a board rewrites it)
        shutil.copy(os.path.join(HW, 'rp569.kicad_pro'), os.path.splitext(pcb)[0] + '.kicad_pro')
    brd = pcbnew.LoadBoard(pcb)
    if not keep:
        for t in list(brd.GetTracks()):  # start from an unrouted board
            brd.Remove(t)
    for z in list(brd.Zones()):          # only the In1 GND plane goes to the router
        if z.GetLayer() != pcbnew.In1_Cu:
            brd.Remove(z)
    if not keep:
        print('GND fan-out vias:', gnd_fanout(brd))
    if not pcbnew.ExportSpecctraDSN(brd, dsn):
        raise SystemExit('DSN export failed')
    if '(class Power' not in open(dsn).read():   # the .kicad_pro next to the board lost its net classes
        raise SystemExit(f'no net classes in {dsn}: run gen_pro.py / copy rp569.kicad_pro next to the board')
    # stock 2.1.0 ignores -mp and a job timeout stops it mid-pass (ripped-up connections left unrouted): use the
    # patched jar; the timeout (2 min per pass) is only a safety net
    minutes = 2 * passes
    subprocess.run(['java', '-jar', JAR, '-de', dsn, '-do', ses, '-mp', str(passes),
                    f'--router.job_timeout={minutes // 60:02d}:{minutes % 60:02d}:00', '--gui.enabled=false'], check=True)
    if not pcbnew.ImportSpecctraSES(brd, ses):
        raise SystemExit('SES import failed')
    gnd = brd.FindNet('/GND')
    for layer in (pcbnew.In2_Cu, pcbnew.F_Cu, pcbnew.B_Cu):    # GND fill on the routing layers
        add_gnd_zone(brd, layer, gnd)
    pcbnew.ZONE_FILLER(brd).Fill(brd.Zones())
    brd.BuildConnectivity()
    left = brd.GetConnectivity().GetUnconnectedCount(False)
    brd.Save(pcb)
    print(f'{len(brd.GetTracks())} track/via items, {left} unrouted -> {pcb}')


if __name__ == '__main__':
    main()
