"""JLCPCB / LCSC part lookup from the jlcparts static data (the LCSC API refuses scripts).

    python3 jlc.py "0402 10kω ±1% chip resistor" "tps2121" ...   -> candidates, Basic first, then by stock
    python3 jlc.py --info C25744 C485916                          -> one line per LCSC number

Data is cached in ~/.cache/jlcparts (search index ~7 MB, attribute table ~1 MB, shards fetched on demand).
Delete the cache to refresh stock numbers.
"""
import gzip
import json
import os
import sys
import urllib.request

BASE = 'https://yaqwsx.github.io/jlcparts/data/'
CACHE = os.path.expanduser('~/.cache/jlcparts')
os.makedirs(os.path.join(CACHE, 'shards'), exist_ok=True)
for _f in ('search-index.tsv.gz', 'attributes-lut.json.gz'):
    if not os.path.exists(os.path.join(CACHE, _f)):
        urllib.request.urlretrieve(BASE + _f, os.path.join(CACHE, _f))

rows = [line.rstrip('\n').split('\t') for line in gzip.open(os.path.join(CACHE, 'search-index.tsv.gz'), 'rt')]
shard_of = {r[0]: r[1] for r in rows}
lut = json.load(gzip.open(os.path.join(CACHE, 'attributes-lut.json.gz')))
_shards = {}


def shard(name):
    if name not in _shards:
        path = os.path.join(CACHE, 'shards', name)
        if not os.path.exists(path):
            urllib.request.urlretrieve(BASE + name, path)
        _shards[name] = {r[0]: r for r in (json.loads(x) for x in gzip.open(path, 'rt') if x.startswith('['))}
    return _shards[name]


def info(lcsc):
    """dict(lcsc, mfr, desc, stock, cls, price, maker, datasheet) or None."""
    r = shard(shard_of[lcsc]).get(lcsc) if lcsc in shard_of else None
    if not r:
        return None
    attrs = {lut[i][0]: lut[i][1] for i in r[8]}

    def attr(name):
        v = attrs.get(name, {}).get('values', {})
        return next(iter(v.values()))[0] if v else '?'
    return dict(lcsc=lcsc, mfr=r[1], desc=r[3][:110], stock=r[9], cls=attr('Basic/Extended'),
                price=r[5][0]['price'] if r[5] else None, maker=attr('Manufacturer'), datasheet=r[4])


def search(query, n=8):
    """Every term must appear in the search text (lower case; use 'ω' for ohms, '±1%', package like '0402')."""
    terms = query.lower().split()
    hits = [r[0] for r in rows if all(t in r[2] for t in terms)]
    found = [i for i in (info(c) for c in hits[:400]) if i]
    found.sort(key=lambda i: (i['cls'] != 'Basic', -i['stock']))
    return found[:n], len(hits)


def show(i):
    print(f"   {i['lcsc']:>10} {i['cls'][:5]:5} stock={i['stock']:>8} ${i['price']} {i['mfr']} ({i['maker']}) | {i['desc']}")


if __name__ == '__main__':
    args = sys.argv[1:]
    if args and args[0] == '--info':
        for c in args[1:]:
            i = info(c)
            show(i) if i else print(f'   {c}: not in the JLCPCB catalogue snapshot')
    for q in ([] if args[:1] == ['--info'] else args):
        res, n = search(q)
        print(f'== {q} ({n} hits)')
        for i in res:
            show(i)
