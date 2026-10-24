"""Check that every firmware command has a Python method, and that no Python method wraps a missing command.

    python3 tools/check_commands.py

Firmware side: the command table between COMMANDS-BEGIN / COMMANDS-END ({"name", ...} entries).
Python side: methods marked @command("name") in the matching module (parsed with ast, nothing is imported).
Run by the pre-commit hook (.githooks/pre-commit); exit code 1 on any difference.
"""
import ast
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAIRS = [
    ('RP2350B', 'firmware/rp2350/src/main.c', 'python/rp569/rp2350.py'),
    ('CH569W', 'firmware/ch569/src/console.c', 'python/rp569/ch569.py'),
]


def firmware_commands(path):
    text = open(os.path.join(ROOT, path)).read()
    m = re.search(r'^\s*(?://|/\*)\s*COMMANDS-BEGIN(.*?)^\s*(?://|/\*)\s*COMMANDS-END', text, re.S | re.M)
    if not m:
        sys.exit(f'{path}: no COMMANDS-BEGIN / COMMANDS-END block')
    return set(re.findall(r'\{\s*"([^"]+)"\s*,', m.group(1)))


def python_commands(path):
    tree = ast.parse(open(os.path.join(ROOT, path)).read(), path)
    names = set()
    for node in ast.walk(tree):
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            for d in node.decorator_list:
                if (isinstance(d, ast.Call) and getattr(d.func, 'id', None) == 'command' and d.args
                        and isinstance(d.args[0], ast.Constant)):
                    names.add(d.args[0].value)
    return names


def main():
    bad = 0
    for chip, fw, py in PAIRS:
        f, p = firmware_commands(fw), python_commands(py)
        for name in sorted(f - p):
            print(f'{chip}: firmware command {name!r} ({fw}) has no @command({name!r}) method in {py}')
            bad += 1
        for name in sorted(p - f):
            print(f'{chip}: {py} wraps {name!r}, which is not in the firmware command table ({fw})')
            bad += 1
        if not (f - p) and not (p - f):
            print(f'{chip}: {len(f)} commands, python library in sync')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
