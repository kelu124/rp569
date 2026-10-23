"""Generate hardware/rp569.kicad_sch from board.py (label-per-pin, one sheet)."""
import copy
import os
import uuid as _uuid

import board
from kisym import find, get_symbol, pins, unq

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GRID = 1.27
STUB = 2.54
NS = _uuid.UUID('6f1c1a52-6a0b-4c55-9d8e-2b7b6e0d5690')


def uid(*key):
    return str(_uuid.uuid5(NS, '/'.join(map(str, key))))


ROOT = uid('root')


def libfile(lib_id):
    lib, _ = lib_id.split(':')
    if lib == 'rp569':
        return board.CH569_LIB
    return board.SYMLIB + lib + '.kicad_sym'


def snap(v):
    return round(round(v / GRID) * GRID, 4)


def q(s):
    return '"' + str(s).replace('"', '\\"') + '"'


def fmt(x, ind=1):
    """Pretty-print S-expression list (tabs, KiCad style)."""
    if not isinstance(x, list):
        return x
    simple = all(not isinstance(i, list) for i in x)
    if simple and len(x) <= 6:
        return '(' + ' '.join(x) + ')'
    out = '(' + x[0]
    for i in x[1:]:
        if isinstance(i, list):
            out += '\n' + '\t' * ind + fmt(i, ind + 1)
        else:
            out += ' ' + i
    return out + '\n' + '\t' * (ind - 1) + ')'


OUT = {0: (-1, 0), 180: (1, 0), 90: (0, 1), 270: (0, -1)}   # lib pin angle -> outward dir (screen)
LABEL_ROT = {(-1, 0): 180, (1, 0): 0, (0, 1): 270, (0, -1): 90}


def effects(size=1.27, hide=False, justify=None):
    e = ['effects', ['font', ['size', str(size), str(size)]]]
    if justify:
        e.append(['justify'] + justify.split())
    if hide:
        e.append(['hide', 'yes'])
    return e


def main():
    lib_syms, sym_pins = {}, {}
    for p in board.parts:
        if p['lib_id'] not in lib_syms:
            name = p['lib_id'].split(':')[1]
            s = get_symbol(libfile(p['lib_id']), name)
            s = copy.deepcopy(s)
            s[1] = q(p['lib_id'])
            lib_syms[p['lib_id']] = s
            sym_pins[p['lib_id']] = pins(s)

    items = []
    blocks = []
    for p in board.parts:
        if p['block'] not in blocks:
            blocks.append(p['block'])

    # block regions: each block packed in a column band, shelves left->right
    x0, y_top = 25.4, 25.4
    col_w = 230.0
    bx = x0
    row_w = 0
    for bi, blk in enumerate(blocks):
        members = [p for p in board.parts if p['block'] == blk]
        cx, cy, shelf_h = bx, y_top + 12.7, 0
        items.append(['text', q(blk), ['exclude_from_sim', 'no'], ['at', str(bx), str(y_top), '0'],
                      effects(3.0, justify='left bottom'), ['uuid', q(uid('blk', blk))]])
        for p in members:
            ps = sym_pins[p['lib_id']]
            # extents incl. stub + label length
            minx = miny = 1e9
            maxx = maxy = -1e9
            for num, name, px, py, ang, ln, typ, unit in ps:
                net = p['pins'].get(num, '')
                dx, dy = OUT[int(ang) % 360]
                lab = STUB + (len(net) * 1.1 + 2 if net else 1)
                ex, ey = px + dx * lab, -py + dy * lab
                for X, Y in ((px, -py), (ex, ey)):
                    minx, maxx, miny, maxy = min(minx, X), max(maxx, X), min(miny, Y), max(maxy, Y)
            if not ps:
                minx, maxx, miny, maxy = -3, 3, -3, 3
            minx -= 3; maxx += 3; miny -= 5; maxy += 5
            w, h = maxx - minx, maxy - miny
            if cx + w > bx + col_w:
                cx, cy, shelf_h = bx, cy + shelf_h + 5, 0
            ox, oy = snap(cx - minx), snap(cy - miny)
            cx += w + 2.54
            shelf_h = max(shelf_h, h)
            p['_sch_at'] = (ox, oy)

            props = [
                ['property', q('Reference'), q(p['ref']), ['at', str(ox), str(snap(oy + miny + 2)), '0'],
                 effects(hide=p['ref'].startswith('#'))],
                ['property', q('Value'), q(p['value']), ['at', str(ox), str(snap(oy + maxy - 2)), '0'],
                 effects(hide=p['ref'].startswith('#'))],
                ['property', q('Footprint'), q(p['fp']), ['at', str(ox), str(oy), '0'], effects(hide=True)],
                ['property', q('Datasheet'), '""', ['at', str(ox), str(oy), '0'], effects(hide=True)],
            ]
            sym = ['symbol', ['lib_id', q(p['lib_id'])], ['at', str(ox), str(oy), '0'], ['unit', '1'],
                   ['exclude_from_sim', 'no'], ['in_bom', 'no' if p['ref'][0] in '#H' else 'yes'],
                   ['on_board', 'no' if p['ref'].startswith('#') else 'yes'],
                   ['dnp', 'yes' if p['dnp'] else 'no'], ['uuid', q(uid('sym', p['ref']))]] + props
            for num in sorted({x[0] for x in ps}):
                sym.append(['pin', q(num), ['uuid', q(uid('pin', p['ref'], num))]])
            sym.append(['instances', ['project', q('rp569'),
                                      ['path', q('/' + ROOT), ['reference', q(p['ref'])], ['unit', '1']]]])
            items.append(sym)

            # labels / no-connects
            used_pts = {}
            for num, name, px, py, ang, ln, typ, unit in ps:
                pt = (snap(ox + px), snap(oy - py))
                net = p['pins'].get(num)
                if net:
                    used_pts.setdefault(pt, set()).add((net, int(ang) % 360))
            done = set()
            for num, name, px, py, ang, ln, typ, unit in ps:
                pt = (snap(ox + px), snap(oy - py))
                net = p['pins'].get(num)
                if not net:
                    if pt not in used_pts and ('nc', pt) not in done:
                        done.add(('nc', pt))
                        items.append(['no_connect', ['at', str(pt[0]), str(pt[1])],
                                      ['uuid', q(uid('nc', p['ref'], num))]])
                    continue
                if (net, pt) in done:
                    continue
                done.add((net, pt))
                d = OUT[int(ang) % 360]
                end = (snap(pt[0] + d[0] * STUB), snap(pt[1] + d[1] * STUB))
                items.append(['wire', ['pts', ['xy', str(pt[0]), str(pt[1])], ['xy', str(end[0]), str(end[1])]],
                              ['stroke', ['width', '0'], ['type', 'default']],
                              ['uuid', q(uid('w', p['ref'], num))]])
                items.append(['label', q(net), ['at', str(end[0]), str(end[1]), str(LABEL_ROT[d])],
                              effects(justify='left bottom' if LABEL_ROT[d] in (0, 90) else 'right bottom'),
                              ['uuid', q(uid('l', p['ref'], num))]])
        bx += col_w + 25.4
        if bi == 2:   # second row
            row_w = bx
            bx, y_top = x0, y_top + 330

    doc = ['kicad_sch', ['version', '20250114'], ['generator', '"rp569_gen"'], ['generator_version', '"9.0"'],
           ['uuid', q(ROOT)], ['paper', '"User"', str(round(max(bx, row_w) + 25)), str(round(y_top + 360))],
           ['title_block', ['title', '"rp569 - RP2350B x CH569W"'], ['rev', '"0.1"'],
            ['comment', '1', '"Generated by hardware/gen/gen_sch.py from board.py - do not edit by hand"']],
           ['lib_symbols'] + list(lib_syms.values())] + items + [
        ['sheet_instances', ['path', '"/"', ['page', '"1"']]], ['embedded_fonts', 'no']]
    with open(os.path.join(HW, 'rp569.kicad_sch'), 'w') as f:
        f.write(fmt(doc) + '\n')
    print(f'{len(board.parts)} parts, {len(items)} items, sheet width {bx:.0f} mm')


if __name__ == '__main__':
    main()
