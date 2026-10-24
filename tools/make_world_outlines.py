"""Country and province outlines for the passport's map, from Natural Earth (public domain).

Reads ne_50m_admin_0_countries.geojson and ne_10m_admin_1_states_provinces.geojson (from
https://github.com/nvkelso/natural-earth-vector, folder geojson), simplifies every ring (Douglas-Peucker), drops the
specks, and writes data/world/countries.txt and data/world/provinces.txt:

    area NAME
    ring x y x y ...      corners in hundredths of a degree (longitude, latitude)

Usage: py -3 -I tools/make_world_outlines.py COUNTRIES.geojson PROVINCES.geojson [OUT_DIR]
"""
import json
import os
import sys


def simplify(points, eps):
    """Douglas-Peucker on a closed ring (the first point is kept, the ring stays closed)."""
    if len(points) < 4:
        return points
    keep = [False] * len(points)
    keep[0] = keep[-1] = True
    stack = [(0, len(points) - 1)]
    while stack:
        a, b = stack.pop()
        ax, ay = points[a]
        bx, by = points[b]
        dx, dy = bx - ax, by - ay
        norm = (dx * dx + dy * dy) ** 0.5
        best, at = -1.0, -1
        for i in range(a + 1, b):
            px, py = points[i]
            if norm < 1e-12:
                d = ((px - ax) ** 2 + (py - ay) ** 2) ** 0.5
            else:
                d = abs(dy * px - dx * py + bx * ay - by * ax) / norm
            if d > best:
                best, at = d, i
        if at >= 0 and best > eps:
            keep[at] = True
            stack.append((a, at))
            stack.append((at, b))
    return [p for p, k in zip(points, keep) if k]


def area(points):
    """The ring's area in square degrees."""
    s = 0.0
    for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1]):
        s += x0 * y1 - x1 * y0
    return abs(s) * 0.5


def rings(geometry):
    """The outer rings and holes of a polygon or multipolygon."""
    if geometry is None:
        return []
    if geometry['type'] == 'Polygon':
        return geometry['coordinates']
    if geometry['type'] == 'MultiPolygon':
        return [r for poly in geometry['coordinates'] for r in poly]
    return []


def convert(path, name_keys, eps, speck, out_path, title):
    with open(path, encoding='utf-8') as f:
        features = json.load(f)['features']
    lines = ['# ' + title, '# From Natural Earth (public domain), simplified by tools/make_world_outlines.py.',
             '# "area NAME", then its rings: corners in hundredths of a degree (longitude latitude).']
    corners = 0
    for feat in features:
        props = feat.get('properties') or {}
        name = next((str(props[k]) for k in name_keys if props.get(k)), '?').upper().replace('|', ' ')
        kept = []
        for ring in rings(feat.get('geometry')):
            pts = [(float(p[0]), float(p[1])) for p in ring]
            if len(pts) > 1 and pts[0] == pts[-1]:
                pts = pts[:-1]
            if len(pts) < 3:
                continue
            a = area(pts)
            s = simplify(pts + [pts[0]], eps)[:-1]
            if len(s) >= 3:
                kept.append((a, s))
        if not kept:
            continue
        biggest = max(a for a, _ in kept)
        kept = [(a, s) for a, s in kept if a >= speck or a == biggest]
        lines.append('area ' + name)
        for _, s in kept:
            lines.append('ring ' + ' '.join('%d %d' % (round(x * 100), round(y * 100)) for x, y in s))
            corners += len(s)
    with open(out_path, 'w', encoding='ascii', errors='replace', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')
    print('%s: %d areas, %d corners, %d KB' % (out_path, sum(1 for l in lines if l.startswith('area ')), corners,
                                               os.path.getsize(out_path) // 1024))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    out = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', 'world')
    os.makedirs(out, exist_ok=True)
    convert(sys.argv[1], ['ADMIN', 'NAME', 'NAME_EN'], 0.05, 0.02, os.path.join(out, 'countries.txt'), 'The countries for the passport\'s map.')
    convert(sys.argv[2], ['name_en', 'name', 'woe_name'], 0.09, 0.04, os.path.join(out, 'provinces.txt'),
            'The states and provinces for the passport\'s map.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
