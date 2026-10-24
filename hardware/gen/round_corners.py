"""Round the board corners: each 45-degree chamfer on Edge.Cuts becomes a tangent arc (routed board, in place).

    /usr/bin/python3 hardware/gen/round_corners.py [board.kicad_pcb]

Default board: hardware/rp569-routed.kicad_pcb. A chamfer from (x0, y0) to (x1, y1) with legs c becomes an arc of
radius c through the same two points, centred c inward from both edges. The arc lies outside the chamfer line, so
the board only gains area: no part, track or zone moves. Diagonal Edge.Cuts segments only; re-running is a no-op.
"""
import math
import os
import sys

import pcbnew

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HW, 'rp569-routed.kicad_pcb')
    board = pcbnew.LoadBoard(path)
    bb = board.GetBoardEdgesBoundingBox()
    cx, cy = bb.GetCenter().x, bb.GetCenter().y
    n = 0
    for d in list(board.GetDrawings()):
        if d.GetLayer() != pcbnew.Edge_Cuts or d.GetShape() != pcbnew.SHAPE_T_SEGMENT:
            continue
        a, b = d.GetStart(), d.GetEnd()
        if a.x == b.x or a.y == b.y:
            continue
        # the point on the horizontal edge keeps its x, the one on the vertical edge keeps its y
        h, v = (a, b) if abs(a.y - cy) > abs(b.y - cy) else (b, a)
        centre = pcbnew.VECTOR2I(h.x, v.y)
        r = math.hypot(h.x - centre.x, h.y - centre.y)
        sx = -1 if h.x < cx else 1
        sy = -1 if v.y < cy else 1
        mid = pcbnew.VECTOR2I(int(centre.x + sx * r / math.sqrt(2)), int(centre.y + sy * r / math.sqrt(2)))
        arc = pcbnew.PCB_SHAPE(board)
        arc.SetShape(pcbnew.SHAPE_T_ARC)
        arc.SetArcGeometry(a, mid, b)
        arc.SetLayer(pcbnew.Edge_Cuts)
        arc.SetWidth(d.GetWidth())
        board.Remove(d)
        board.Add(arc)
        n += 1
    board.Save(path)
    print(f'{n} chamfers -> arcs in {path}')


if __name__ == '__main__':
    main()
