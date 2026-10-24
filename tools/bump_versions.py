"""Bump the patch version of every component touched by the commit being made (pre-commit hook).

    python3 tools/bump_versions.py            # from .githooks/pre-commit, on the staged changes
    python3 tools/bump_versions.py --show     # print the current versions

Components: firmware/rp2350 (version.yaml), firmware/ch569 (version.yaml), python (rp569/_version.py).
Rule (major.minor.patch): if a component has staged changes other than its version file, its staged version must
be above HEAD's. If major/minor are unchanged and patch was not raised, patch = HEAD patch + 1 (the file is
rewritten and staged). A changed major or minor is the user's decision and is left as it is.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPONENTS = [
    ('firmware/rp2350', 'firmware/rp2350/version.yaml'),
    ('firmware/ch569', 'firmware/ch569/version.yaml'),
    ('python', 'python/rp569/_version.py'),
]
IGNORED = ('/build/', '__pycache__', '.egg-info')


def git(*args, check=True):
    r = subprocess.run(['git', *args], cwd=ROOT, capture_output=True, text=True)
    if check and r.returncode:
        sys.exit(f'git {" ".join(args)} failed: {r.stderr}')
    return r


def parse(text):
    if text is None:
        return None
    if '__version__' in text:
        m = re.search(r'__version__\s*=\s*"(\d+)\.(\d+)\.(\d+)"', text)
        return tuple(map(int, m.groups())) if m else None
    parts = {k: int(v) for k, v in re.findall(r'^(major|minor|patch):\s*(\d+)', text, re.M)}
    return (parts['major'], parts['minor'], parts['patch']) if len(parts) == 3 else None


def with_patch(text, ver):
    if '__version__' in text:
        return re.sub(r'__version__\s*=\s*"[^"]*"', f'__version__ = "{ver[0]}.{ver[1]}.{ver[2]}"', text)
    return re.sub(r'^patch:\s*\d+', f'patch: {ver[2]}', text, flags=re.M)


def show(path):
    r = git('show', path, check=False)
    return r.stdout if r.returncode == 0 else None


def main():
    if '--show' in sys.argv:
        for comp, vf in COMPONENTS:
            print(f'{comp:16s} {".".join(map(str, parse(open(os.path.join(ROOT, vf)).read())))}')
        return
    staged = git('diff', '--cached', '--name-only').stdout.split()
    for comp, vf in COMPONENTS:
        touched = [f for f in staged if f.startswith(comp + '/') and f != vf and not any(i in f for i in IGNORED)]
        if not touched:
            continue
        head, index = parse(show(f'HEAD:{vf}')), parse(show(f':{vf}'))
        if index is None:
            sys.exit(f'{vf}: missing or unreadable in the index (needs major/minor/patch)')
        if head is None:
            print(f'{comp}: new component at {".".join(map(str, index))}')
            continue
        if index[:2] != head[:2]:
            print(f'{comp}: {".".join(map(str, head))} -> {".".join(map(str, index))} (major/minor set by the user)')
            continue
        if index[2] > head[2]:
            print(f'{comp}: patch already raised to {index[2]}')
            continue
        work = open(os.path.join(ROOT, vf)).read()
        if work != show(f':{vf}'):
            sys.exit(f'{vf} has unstaged edits: stage or revert them, then commit again')
        new = (head[0], head[1], head[2] + 1)
        open(os.path.join(ROOT, vf), 'w').write(with_patch(work, new))
        git('add', vf)
        print(f'{comp}: {".".join(map(str, head))} -> {".".join(map(str, new))} (patch bump, {len(touched)} files)')


if __name__ == '__main__':
    main()
