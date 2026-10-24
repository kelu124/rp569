"""Board renders for the docs: docs/images/ (from hardware/rp569-routed.kicad_pcb, or a board given as argument).

    python3 hardware/gen/gen_renders.py [board.kicad_pcb] [--tag v1.0]

3D (kicad-cli pcb render, PNG): top, bottom (orthographic) and an angled perspective view of each side.
2D (kicad-cli pcb export svg, board area only): one SVG per copper layer with the board outline, bottom layer
mirrored so it reads as seen from below, plus the top/bottom assembly views (silkscreen + fab + outline).
Each SVG also gets a PNG (white background, 1200 px wide) through Inkscape when it is installed (Hackaday and
most blog tools do not take SVG).
"""
import os
import shutil
import subprocess
import sys

HW = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(os.path.dirname(HW), 'docs', 'images')


def cli(*args):
    r = subprocess.run(['kicad-cli', *args], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f'kicad-cli failed: {" ".join(args[:4])}\n{r.stdout}{r.stderr}')


def main():
    args = sys.argv[1:]
    tag = args[args.index('--tag') + 1] if '--tag' in args else 'v1.0'
    pos = [a for a in args if not a.startswith('--') and a != tag]
    pcb = pos[0] if pos else os.path.join(HW, 'rp569-routed.kicad_pcb')
    os.makedirs(OUT, exist_ok=True)
    name = f'rp569-{tag}'
    views = {
        'top': ['--side', 'top'],
        'bottom': ['--side', 'bottom'],
        'top-angled': ['--side', 'top', '--perspective', '--rotate', '-35,0,25', '--zoom', '0.85', '--floor'],
        'bottom-angled': ['--side', 'bottom', '--perspective', '--rotate', '-35,0,-25', '--zoom', '0.85', '--floor'],
    }
    for view, opts in views.items():
        w, h = (1200, 1600) if view in ('top', 'bottom') else (1600, 1200)
        cli('pcb', 'render', *opts, '--width', str(w), '--height', str(h), '--quality', 'high',
            '--background', 'opaque', '-o', os.path.join(OUT, f'{name}-{view}.png'), pcb)
        print('render', view)
    plots = {
        'F_Cu': ('F.Cu', False), 'In1_Cu': ('In1.Cu', False), 'In2_Cu': ('In2.Cu', False), 'B_Cu': ('B.Cu', True),
        'assembly-top': ('F.Silkscreen,F.Fab', False), 'assembly-bottom': ('B.Silkscreen,B.Fab', True),
    }
    for label, (layers, mirror) in plots.items():
        cli('pcb', 'export', 'svg', '--mode-single', '-l', layers + ',Edge.Cuts', '--page-size-mode', '2',
            '--exclude-drawing-sheet', '--check-zones', *(['--mirror'] if mirror else []),
            '-o', os.path.join(OUT, f'{name}-{label}.svg'), pcb)
        if shutil.which('inkscape'):
            svg = os.path.join(OUT, f'{name}-{label}.svg')
            subprocess.run(['inkscape', svg, '--export-type=png', '--export-width=1200',
                            '--export-background=white', '--export-background-opacity=1',
                            f'--export-filename={svg[:-4]}.png'], check=True, capture_output=True)
        print('plot', label)


if __name__ == '__main__':
    main()
