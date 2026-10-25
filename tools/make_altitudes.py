"""Writes data/altitudes.txt: how high the roads of each place of data/route.txt lie above the sea.

The heights come from the ASTER GDEM (30 m) through the public OpenTopoData API (no key; only the coordinates are
sent; at most a request a second and a hundred points each). Nine points round each place (+-0.15 degrees) are asked
and the third lowest is taken: the roads keep to the valleys and the coast, not to the peaks the place may be named
after. Places with a pass (`summit H NAME`) climb to it in the game anyway.

    python tools/make_altitudes.py

A line of the file may be corrected by hand; running the script again writes every line anew.

@author Rene Weller
"""
import json
import math
import os
import time
import urllib.request

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
SPREAD = 0.15   # degrees of latitude round the place
BATCH = 100     # coordinates per request


def route():
    """The places of the world tour: ID, latitude, longitude."""
    out = []
    for line in open(os.path.join(ROOT, 'data', 'route.txt'), encoding='utf-8'):
        p = line.split()
        if len(p) >= 3 and not line.startswith('#'):
            out.append((p[0], float(p[1]), float(p[2])))
    return out


def ring(lat, lon):
    """Nine points round a place, as far apart east and west as north and south."""
    east = SPREAD / max(0.2, math.cos(math.radians(lat)))
    return [(max(-89.9, min(89.9, lat + dy)), (lon + dx + 180.0) % 360.0 - 180.0)
            for dy in (-SPREAD, 0.0, SPREAD) for dx in (-east, 0.0, east)]


def elevations(points):
    """The heights of the points, a request per hundred (with a few retries)."""
    heights = []
    for i in range(0, len(points), BATCH):
        chunk = points[i:i + BATCH]
        url = 'https://api.opentopodata.org/v1/aster30m?locations=' + '|'.join('%.4f,%.4f' % p for p in chunk)
        for attempt in range(6):
            try:
                with urllib.request.urlopen(url, timeout=60) as answer:
                    got = [r['elevation'] for r in json.load(answer)['results']]
                break
            except Exception as error:   # the service limits its rate: wait and ask again
                print('  waiting:', error)
                time.sleep(10.0 * (attempt + 1))
        else:
            raise SystemExit('the elevation service does not answer')
        heights += [h if isinstance(h, (int, float)) and math.isfinite(h) else 0.0 for h in got]   # the open sea: 0
        print('  %d of %d points' % (len(heights), len(points)))
        time.sleep(1.2)
    return heights


def main():
    places = route()
    points = [p for _, lat, lon in places for p in ring(lat, lon)]
    heights = elevations(points)
    lines = ['# How high the roads of each place lie above the sea (m): the third lowest of nine points round it (+-0.15',
             '# degrees) in the ASTER GDEM, through the OpenTopoData API; written by tools/make_altitudes.py. The road',
             '# climbs and falls towards these heights from place to place.']
    for n, (name, _, _) in enumerate(places):
        lines.append('%s %d' % (name, round(sorted(heights[n * 9:n * 9 + 9])[2])))
    path = os.path.join(ROOT, 'data', 'altitudes.txt')
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    print('wrote', path, len(places), 'places')


main()
