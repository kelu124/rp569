"""Build project footprints in hardware/lib/rp569.pretty/.

USB3_Micro-B_Amphenol_GSB343K33HR: from the Amphenol drawing GSB343K33HR rev C, "Recommended PCB layout - top
view" (datasheet/GSB343K33HR/gsb343k33hr.pdf). Origin = midpoint of the two shell-leg slots, +y toward the board
edge (the plug side), same convention as the KiCad Connfly footprint.
"""
import os

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(HW, 'lib', 'rp569.pretty')

# drawing dimensions, mm
SLOT_DX = 12.55            # slot centre to slot centre
SLOT_W, SLOT_H = 0.50, 1.30
RING = 0.25                # annular ring around the slot (chosen, the drawing does not dimension the copper)
EDGE = 1.45                # slot centre line to PCB edge
PITCH, PAD_W, PAD_L = 0.65, 0.40, 1.35
PAD_TOP = 3.35             # slot centre line to the far end of the SMD pads
GRP1 = -SLOT_DX / 2 + 3.60  # centre of pins 1..5 (pin 3)
GRP2 = GRP1 + 6.70          # centre of pins 6..10 (pin 8)
SHELL_W = 12.25
BODY_BACK, BODY_FRONT = -2.60, EDGE + 1.55   # fab outline, approximate (side view: 2.15 + 0.90 + 3.00)


def f(v):
    return f'{v:.4f}'.rstrip('0').rstrip('.')


def line(layer, x0, y0, x1, y1, w):
    return (f'\t(fp_line (start {f(x0)} {f(y0)}) (end {f(x1)} {f(y1)}) (stroke (width {w}) (type solid)) '
            f'(layer "{layer}"))')


def rect(layer, x0, y0, x1, y1, w):
    return '\n'.join(line(layer, *s, w) for s in ((x0, y0, x1, y0), (x1, y0, x1, y1), (x1, y1, x0, y1), (x0, y1, x0, y0)))


def text(kind, value, y, layer, hide=False):
    h = ' (hide yes)' if hide else ''
    return (f'\t(property "{kind}" "{value}" (at 0 {f(y)} 0) (layer "{layer}"){h}\n'
            f'\t\t(effects (font (size 1 1) (thickness 0.15))))')


def gsb343k33hr():
    name = 'USB3_Micro-B_Amphenol_GSB343K33HR'
    out = [f'(footprint "{name}"', '\t(version 20241229)', '\t(generator "rp569_gen")', '\t(layer "F.Cu")',
           '\t(descr "USB 3.0 Micro-B receptacle, right angle, SMD contacts, through-hole shell legs '
           '(Amphenol GSB343K33HR, drawing rev C)")',
           '\t(tags "USB 3.0 Micro B THT shell")',
           text('Reference', 'REF**', -PAD_TOP - 1.2, 'F.SilkS'),
           text('Value', name, BODY_FRONT + 1.2, 'F.Fab'),
           '\t(attr smd)']
    pad_y = -(PAD_TOP - PAD_L / 2)
    for i in range(5):
        for num, cx in ((i + 1, GRP1), (i + 6, GRP2)):
            x = cx + (i - 2) * PITCH
            out.append(f'\t(pad "{num}" smd rect (at {f(x)} {f(pad_y)}) (size {f(PAD_W)} {f(PAD_L)}) '
                       f'(layers "F.Cu" "F.Paste" "F.Mask"))')
    for x in (-SLOT_DX / 2, SLOT_DX / 2):
        out.append(f'\t(pad "SH" thru_hole oval (at {f(x)} 0) (size {f(SLOT_W + 2 * RING)} {f(SLOT_H + 2 * RING)}) '
                   f'(drill oval {f(SLOT_W)} {f(SLOT_H)}) (layers "*.Cu" "*.Mask"))')
    sx = SHELL_W / 2
    out.append(rect('F.Fab', -sx, BODY_BACK, sx, BODY_FRONT, 0.1))
    out.append(line('Dwgs.User', -sx - 0.5, EDGE, sx + 0.5, EDGE, 0.1))      # PCB edge
    cx = SLOT_DX / 2 + (SLOT_W + 2 * RING) / 2 + 0.25
    out.append(rect('F.CrtYd', -cx, -PAD_TOP - 0.25, cx, BODY_FRONT + 0.25, 0.05))
    # silk: the sides behind the slots and the back edge, clear of the pads
    sy = -PAD_TOP - 0.15
    out.append(line('F.SilkS', -sx - 0.1, sy, GRP1 - 2 * PITCH - 0.5, sy, 0.12))
    out.append(line('F.SilkS', GRP2 + 2 * PITCH + 0.5, sy, sx + 0.1, sy, 0.12))
    out.append(line('F.SilkS', -sx - 0.1, sy, -sx - 0.1, -SLOT_H / 2 - RING - 0.3, 0.12))
    out.append(line('F.SilkS', sx + 0.1, sy, sx + 0.1, -SLOT_H / 2 - RING - 0.3, 0.12))
    out.append(')')
    return name, '\n'.join(out) + '\n'


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, body in (gsb343k33hr(),):
        with open(os.path.join(OUT, name + '.kicad_mod'), 'w') as fh:
            fh.write(body)
        print('footprint', name)


if __name__ == '__main__':
    main()
