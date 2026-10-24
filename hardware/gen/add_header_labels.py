"""Pin labels on the bottom silkscreen next to every 2.54 mm header (routed board).

    /usr/bin/python3 hardware/gen/add_header_labels.py [board.kicad_pcb]

Default board: hardware/rp569-routed.kicad_pcb (edited in place). One mirrored B.SilkS text per pin, on the
inward side of the header (the headers sit ~1 mm from the edge). Column headers (rot 0) get horizontal text
beside each pin; row headers (rot 90) get text above/below each pin, turned vertical when the labels are
wider than the pitch, or 0.8 mm horizontal when the vertical ones would hit a part. The 2x07 CH EXT gets one "inner outer" pair per row beside its inner column.
All labels live in the group 'header_labels': a re-run deletes it and starts again. Prints any label whose
box hits a bottom pad, other bottom silkscreen or the board edge.
"""
import os
import sys

import pcbnew

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GROUP = 'header_labels'
H, T = 0.9, 0.15           # text height / stroke (JLCPCB: >= 0.8 mm / 0.15 mm legible)
GAP = 0.35                 # pad edge -> text box
CX, CY = 25.0, 35.0        # board centre, defines "inward"
REF_FIXUPS = {'U4': (33.5, 5.0, 0.8)}  # reference moved/shrunk to make room: ref -> (x, y, height mm)
SHORT = {'+3V3': '3V3', '3V3_CH': '3V3', 'SWCLK': 'CLK', 'SWDIO': 'DIO', 'CH_BOOT_SW': 'BOOT'}


def mm(v):
    return pcbnew.FromMM(v)


def label(net):
    n = net.lstrip('/')
    if n in SHORT:
        return SHORT[n]
    for p in ('RP_DBG_', 'CH_DBG_', 'RP_I2C_', 'CH_I2C_', 'RP_', 'CH_'):
        if n.startswith(p):
            return n[len(p):]
    return n


def text(board, s, vertical=False, h=H):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetLayer(pcbnew.B_SilkS)
    t.SetMirrored(True)
    t.SetTextSize(pcbnew.VECTOR2I(mm(h), mm(h)))
    t.SetTextThickness(mm(T))
    t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_CENTER)
    t.SetVertJustify(pcbnew.GR_TEXT_V_ALIGN_CENTER)
    if vertical:
        t.SetTextAngleDegrees(90)
    return t


def ink(t):
    return t.GetEffectiveTextShape().BBox()     # drawn strokes; GetBoundingBox() adds line spacing


def size(t):
    bb = ink(t)
    return bb.GetWidth() / 1e6, bb.GetHeight() / 1e6


def place(t, x, y):
    t.SetPosition(pcbnew.VECTOR2I(mm(x), mm(y)))


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HW, 'rp569-routed.kicad_pcb')
    board = pcbnew.LoadBoard(path)
    for g in list(board.Groups()):
        if g.GetName() == GROUP:
            for it in list(g.GetItems()):
                board.Remove(it)
            board.Remove(g)
    group = pcbnew.PCB_GROUP(board)
    group.SetName(GROUP)
    board.Add(group)
    for ref, (x, y, h) in REF_FIXUPS.items():
        f = board.FindFootprintByReference(ref).Reference()
        f.SetTextSize(pcbnew.VECTOR2I(mm(h), mm(h)))
        f.SetTextThickness(mm(min(T, h / 5)))
        place(f, x, y)
    made = []
    # obstacles: bottom pads, other bottom silkscreen, board edge
    edge = board.GetBoardEdgesBoundingBox()
    obst = []
    for fp in board.GetFootprints():
        for p in fp.Pads():
            if p.IsOnLayer(pcbnew.B_Cu):
                obst.append((f'pad {fp.GetReference()}.{p.GetNumber()}', p.GetBoundingBox()))
        for it in fp.GraphicalItems():
            if it.GetLayer() == pcbnew.B_SilkS:
                obst.append((f'silk {fp.GetReference()}', it.GetBoundingBox()))
        for it in (fp.Reference(), fp.Value()):
            if it.GetLayer() == pcbnew.B_SilkS and it.IsVisible():
                obst.append((f'silk {fp.GetReference()}', ink(it)))
    for d in board.GetDrawings():
        if d.GetLayer() == pcbnew.B_SilkS:
            obst.append(('silk drawing', d.GetBoundingBox()))

    def hits(t):
        bb = ink(t)
        out = sorted({n for n, o in obst if bb.Intersects(o)})
        return out + ([] if edge.Contains(bb) else ['board edge'])

    for fp in board.GetFootprints():
        if 'PinHeader' not in str(fp.GetFPID().GetLibItemName()):
            continue
        pads = sorted(fp.Pads(), key=lambda p: int(p.GetNumber()))
        xy = [(p.GetPosition().x / 1e6, p.GetPosition().y / 1e6) for p in pads]
        bb0 = pads[0].GetBoundingBox()
        r = max(bb0.GetWidth(), bb0.GetHeight()) / 2e6
        names = [label(p.GetNetname()) for p in pads]
        column = abs(xy[0][0] - xy[-1][0]) < abs(xy[0][1] - xy[-1][1])   # pins stacked along y
        if column:
            xs = sorted({round(x, 2) for x, _ in xy})
            inner = max(xs, key=lambda x: -abs(x - CX))                  # column nearest the centre
            d = 1 if CX > inner else -1
            rows = {}
            for (x, y), n in zip(xy, names):
                rows.setdefault(round(y, 2), []).append((abs(x - inner), n))
            for y, ns in rows.items():
                s = ' '.join(n for _, n in sorted(ns))                   # inner pin first
                t = text(board, s)
                w, _ = size(t)
                place(t, inner + d * (r + GAP + w / 2), y)
                made.append(t)
        else:
            y0 = xy[0][1]
            d = 1 if CY > y0 else -1
            # horizontal if it fits the pitch, else vertical, else smaller horizontal (first one without hits)
            for vert, h, maxw in ((False, H, 2.2), (True, H, 2.54), (False, 0.8, 2.3)):
                ts = [text(board, n, vert, h) for n in names]
                if max(size(t)[0] for t in ts) > maxw:
                    continue
                for (x, _), t in zip(xy, ts):
                    place(t, x, y0 + d * (r + GAP + size(t)[1] / 2))
                if not any(hits(t) for t in ts):
                    break
            made += ts

    for t in made:
        board.Add(t)
        group.AddItem(t)

    bad = 0
    for t in made:
        if hits(t):
            bad += 1
            c = ink(t).GetCenter()
            print(f'  {t.GetText():10s} at ({c.x / 1e6:.1f}, {c.y / 1e6:.1f}): {", ".join(hits(t))}')
    board.Save(path)
    print(f'{len(made)} labels in group {GROUP!r}, {bad} with overlaps -> {path}')


if __name__ == '__main__':
    main()
