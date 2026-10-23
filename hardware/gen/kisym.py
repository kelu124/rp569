"""Minimal KiCad S-expression reader/writer + symbol library helpers."""
import re

def tokenize(s):
    for m in re.finditer(r'\(|\)|"(?:\\.|[^"\\])*"|[^\s()"]+', s):
        yield m.group(0)

def parse(s):
    stack = [[]]
    for t in tokenize(s):
        if t == '(':
            stack.append([])
        elif t == ')':
            x = stack.pop(); stack[-1].append(x)
        else:
            stack[-1].append(t)
    return stack[0][0]

def unq(t):
    return t[1:-1].replace('\\"', '"') if isinstance(t, str) and t.startswith('"') else t

def dump(x, ind=0):
    if not isinstance(x, list):
        return x
    if all(not isinstance(i, list) for i in x):
        return '(' + ' '.join(x) + ')'
    out = '(' + ' '.join(i for i in x[:1])
    for i in x[1:]:
        out += '\n' + '  ' * (ind + 1) + dump(i, ind + 1) if isinstance(i, list) else ' ' + i
    return out + ')'

def find(x, key):
    return [i for i in x if isinstance(i, list) and i and i[0] == key]

_cache = {}
def load_lib(path):
    if path not in _cache:
        root = parse(open(path).read())
        _cache[path] = {unq(s[1]): s for s in find(root, 'symbol')}
    return _cache[path]

def get_symbol(path, name):
    """Return a flattened (no 'extends') deep copy of a library symbol."""
    import copy
    lib = load_lib(path)
    sym = copy.deepcopy(lib[name])
    ext = find(sym, 'extends')
    if ext:
        parent = get_symbol(path, unq(ext[0][1]))
        pname = unq(parent[1])
        props = {unq(p[1]): p for p in find(sym, 'property')}
        new = [parent[0], f'"{name}"']
        for item in parent[2:]:
            if isinstance(item, list) and item[0] == 'property' and unq(item[1]) in props:
                new.append(props.pop(unq(item[1])))
            elif isinstance(item, list) and item[0] == 'symbol':
                sub = copy.deepcopy(item)
                sub[1] = '"' + unq(sub[1]).replace(pname, name, 1) + '"'
                new.append(sub)
            else:
                new.append(item)
        # insert extra props before sub-symbols
        idx = next(i for i, it in enumerate(new) if isinstance(it, list) and it[0] == 'symbol')
        for p in props.values():
            new.insert(idx, p)
        sym = new
    return sym

def pins(sym):
    """[(number, name, x, y, angle, length, type)] for all units."""
    out = []
    for sub in find(sym, 'symbol'):
        for p in find(sub, 'pin'):
            at = find(p, 'at')[0]
            ln = find(p, 'length')[0]
            name = unq(find(p, 'name')[0][1]); num = unq(find(p, 'number')[0][1])
            out.append((num, name, float(at[1]), float(at[2]), float(at[3]) if len(at) > 3 else 0.0,
                        float(ln[1]), p[1], unq(sub[1])))
    return out
