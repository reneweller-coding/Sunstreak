"""Place names in their own script for the place signs, as strokes traced from the Noto fonts.

Reads the 'local' lines of data/places/*.txt, sets each name in the Noto font of its script (shaped by HarfBuzz
through Pillow's raqm layout: joined Arabic, Devanagari conjuncts, right to left where the script reads so), thins
the letters to their centre lines (Zhang-Suen), follows those lines into strokes, simplifies them (Douglas-Peucker)
and writes data/script-names.txt:

    ID|width|x,y x,y x,y;x,y x,y ...      in 64ths of an em, x from the left of the ink, y up from the baseline

The Noto fonts are under the SIL Open Font License 1.1 (docs/OFL-Noto.txt), and so are the strokes traced from them.
The fonts are looked for where Android Studio, its SDK and MiKTeX install them (or in the folders given).

Usage: py -3 tools/make_signs.py [FONT_DIR ...]
"""
import glob
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont, features

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EM = 160            # pixels per em while tracing
UNIT = 64           # stored units per em
FONT_DIRS = [
    r'C:\Program Files\Android\Android Studio\plugins\design-tools\resources\layoutlib\data\fonts',
    r'C:\Program Files (x86)\Android\android-sdk\platforms\android-19\data\fonts',
    os.path.expandvars(r'%LOCALAPPDATA%\Programs\MiKTeX\fonts\truetype\google\noto'),
]

# Unicode blocks and the font files that set them (the first found wins).
SCRIPTS = [
    ((0x0590, 0x05FF), ['NotoSansHebrew-Regular.ttf']),
    ((0x0600, 0x06FF), ['NotoNaskhArabic-Regular.ttf']),
    ((0x0780, 0x07BF), ['NotoSansThaana-Regular.ttf', 'NotoSansThaana-Bold.ttf']),
    ((0x0900, 0x097F), ['NotoSansDevanagari-VF.ttf', 'NotoSansDevanagari-Regular.ttf']),
    ((0x0980, 0x09FF), ['NotoSansBengali-VF.ttf', 'NotoSansBengali-Regular.ttf']),
    ((0x0A00, 0x0A7F), ['NotoSansGurmukhi-VF.ttf']),
    ((0x0A80, 0x0AFF), ['NotoSansGujarati-Regular.ttf']),
    ((0x0B00, 0x0B7F), ['NotoSansOriya-Regular.ttf']),
    ((0x0B80, 0x0BFF), ['NotoSansTamil-VF.ttf', 'NotoSansTamil-Regular.ttf']),
    ((0x0C00, 0x0C7F), ['NotoSansTelugu-VF.ttf', 'NotoSansTelugu-Regular.ttf']),
    ((0x0C80, 0x0CFF), ['NotoSansKannada-VF.ttf', 'NotoSansKannada-Regular.ttf']),
    ((0x0D00, 0x0D7F), ['NotoSansMalayalam-VF.ttf', 'NotoSansMalayalam-Regular.ttf']),
    ((0x0D80, 0x0DFF), ['NotoSansSinhala-VF.ttf']),
    ((0x0E00, 0x0E7F), ['NotoSansThai-Regular.ttf']),
    ((0x0E80, 0x0EFF), ['NotoSansLao-Regular.ttf']),
    ((0x0F00, 0x0FFF), ['NotoSerifTibetan-VF.ttf']),
    ((0x1000, 0x109F), ['NotoSansMyanmar-Regular.otf']),
    ((0x10A0, 0x10FF), ['NotoSansGeorgian-VF.ttf']),
    ((0x1200, 0x137F), ['NotoSansEthiopic-VF.ttf']),
    ((0x1780, 0x17FF), ['NotoSansKhmer-VF.ttf']),
    ((0x0530, 0x058F), ['NotoSansArmenian-VF.ttf']),
    ((0x0370, 0x04FF), ['NotoSans-Regular.ttf']),
]
CJK = 'NotoSansCJK-Regular.ttc'
CJK_FACE = {'JAPAN': 'JP', 'SOUTH KOREA': 'KR', 'NORTH KOREA': 'KR', 'TAIWAN': 'TC', 'CHINA': 'SC'}
CJK_REGION_FACE = {'HONG KONG': 'HK', 'MACAU': 'TC'}


def find_font(names, dirs):
    for name in names:
        for d in dirs:
            p = os.path.join(d, name)
            if os.path.exists(p):
                return p
    return None


def cjk_index(path, face):
    """The face of the CJK collection for a region (JP, KR, SC, TC, HK)."""
    for i in range(10):
        try:
            name = ImageFont.truetype(path, 12, index=i).getname()[0]
        except OSError:
            break
        if name.endswith(' ' + face) and 'Mono' not in name:
            return i
    return 0


def font_for(text, country, region, dirs):
    """The font file and face for a name, by the script of its first letter (and for CJK the country)."""
    for ch in text:
        c = ord(ch)
        if 0x1100 <= c <= 0x11FF or 0x3040 <= c <= 0x30FF or 0x3130 <= c <= 0x318F or 0x4E00 <= c <= 0x9FFF or 0xAC00 <= c <= 0xD7AF:
            path = find_font([CJK], dirs)
            face = CJK_REGION_FACE.get(region, CJK_FACE.get(country, 'SC'))
            if 0xAC00 <= c <= 0xD7AF or 0x1100 <= c <= 0x11FF or 0x3130 <= c <= 0x318F:
                face = 'KR'
            return path, cjk_index(path, face) if path else 0
        for (lo, hi), names in SCRIPTS:
            if lo <= c <= hi:
                return find_font(names, dirs), 0
    return None, 0


def thin(img):
    """Zhang-Suen thinning of a 0/1 image down to lines one pixel wide."""
    img = img.astype(np.uint8).copy()
    while True:
        changed = False
        for step in (0, 1):
            P = np.pad(img, 1)
            p2, p3, p4, p5 = P[:-2, 1:-1], P[:-2, 2:], P[1:-1, 2:], P[2:, 2:]
            p6, p7, p8, p9 = P[2:, 1:-1], P[2:, :-2], P[1:-1, :-2], P[:-2, :-2]
            ring = [p2, p3, p4, p5, p6, p7, p8, p9, p2]
            b = sum(r.astype(np.int32) for r in ring[:8])
            a = sum(((ring[i] == 0) & (ring[i + 1] == 1)).astype(np.int32) for i in range(8))
            if step == 0:
                m = (p2 * p4 * p6 == 0) & (p4 * p6 * p8 == 0)
            else:
                m = (p2 * p4 * p8 == 0) & (p2 * p6 * p8 == 0)
            gone = (img == 1) & (b >= 2) & (b <= 6) & (a == 1) & m
            if gone.any():
                img[gone] = 0
                changed = True
        if not changed:
            return img


NEIGH = [(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)]


def trace(sk):
    """Follows the one-pixel lines into polylines from end to junction, junction to junction, round loops."""
    ys, xs = np.nonzero(sk)
    on = set(zip(ys.tolist(), xs.tolist()))
    nb = {p: [(p[0] + dy, p[1] + dx) for dy, dx in NEIGH if (p[0] + dy, p[1] + dx) in on] for p in on}
    nodes = {p for p in on if len(nb[p]) != 2}
    used = set()
    paths = []

    def walk(start, nxt):
        path = [start, nxt]
        used.add(frozenset((start, nxt)))
        prev, cur = start, nxt
        while cur not in nodes:
            step = [q for q in nb[cur] if q != prev and frozenset((cur, q)) not in used]
            if not step:
                break
            prev, cur = cur, step[0]
            used.add(frozenset((prev, cur)))
            path.append(cur)
            if cur == start:
                break
        return path

    for p in sorted(nodes):
        if not nb[p]:
            paths.append([p, p])   # a dot
        for q in nb[p]:
            if frozenset((p, q)) not in used:
                paths.append(walk(p, q))
    for p in sorted(on):            # loops with no ends or junctions (an O)
        for q in nb[p]:
            if frozenset((p, q)) not in used:
                paths.append(walk(p, q))
    # Thinning leaves short spurs where a stroke's end was square; the links between junction pixels (staircase
    # diagonals) stay, or diagonals would break.
    out = []
    for path in paths:
        a, b = path[0], path[-1]
        if (len(nb[a]) == 1) != (len(nb[b]) == 1) and len(path) < EM * 0.035:
            continue
        out.append(path)
    return out


def simplify(pts, eps):
    """Douglas-Peucker on an open polyline."""
    if len(pts) < 3:
        return pts
    keep = [False] * len(pts)
    keep[0] = keep[-1] = True
    stack = [(0, len(pts) - 1)]
    while stack:
        i, j = stack.pop()
        (ay, ax), (by, bx) = pts[i], pts[j]
        dy, dx = by - ay, bx - ax
        norm = (dx * dx + dy * dy) ** 0.5
        best, at = -1.0, -1
        for k in range(i + 1, j):
            py, px = pts[k]
            d = abs(dx * (ay - py) - dy * (ax - px)) / norm if norm > 1e-9 else ((py - ay) ** 2 + (px - ax) ** 2) ** 0.5
            if d > best:
                best, at = d, k
        if best > eps:
            keep[at] = True
            stack += [(i, at), (at, j)]
    return [p for p, k in zip(pts, keep) if k]


def strokes(text, path, index):
    """The strokes of a name: (width in units, polylines in units)."""
    font = ImageFont.truetype(path, EM, index=index, layout_engine=ImageFont.Layout.RAQM)
    ascent, descent = font.getmetrics()
    left, top, right, bottom = ImageDraw.Draw(Image.new('L', (1, 1))).textbbox((0, 0), text, font=font, anchor='ls')
    pad = 8
    w, h = right - left + 2 * pad, ascent + descent + 2 * pad
    img = Image.new('L', (w, h), 0)
    base = pad + ascent
    ImageDraw.Draw(img).text((pad - left, base), text, font=font, fill=255, anchor='ls')
    sk = thin(np.array(img) > 110)
    lines = []
    for p in trace(sk):
        q = simplify(p, 1.4)
        lines.append([(round((x - pad) * UNIT / EM), round((base - y) * UNIT / EM)) for y, x in q])
    return round((right - left) * UNIT / EM), lines


def places(data):
    """Every place with a 'local' line: (id, local name, country, region)."""
    out = []
    for path in sorted(glob.glob(os.path.join(data, 'places', '*.txt'))):
        current = None
        for line in open(path, encoding='utf-8'):
            if line.startswith('place '):
                parts = [s.strip() for s in line.split('|')]
                current = (parts[0].split()[1], parts[2] if len(parts) > 2 else '', parts[4] if len(parts) > 4 else '')
            elif current and line.strip().startswith('local '):
                out.append((current[0], line.strip()[6:].strip(), current[1], current[2]))
    return out


def main():
    if not features.check('raqm'):
        sys.exit('Pillow lacks raqm (HarfBuzz): Arabic and the Indic scripts would not be shaped')
    dirs = sys.argv[1:] + FONT_DIRS
    data = os.path.join(ROOT, 'data')
    rows = ['# Place names in their own script, traced from the Noto fonts (SIL Open Font License 1.1, docs/OFL-Noto.txt)',
            '# by tools/make_signs.py from the places\' local lines; do not edit.',
            '# ID|width|x,y x,y;x,y ...   in 64ths of an em, x from the left of the ink, y up from the baseline']
    missing = []
    for pid, text, country, region in places(data):
        path, index = font_for(text, country, region, dirs)
        if not path:
            missing.append(pid)
            continue
        width, lines = strokes(text, path, index)
        body = ';'.join(' '.join(f'{x},{y}' for x, y in line) for line in lines)
        rows.append(f'{pid}|{width}|{body}')
    open(os.path.join(data, 'script-names.txt'), 'w', encoding='utf-8', newline='\n').write('\n'.join(rows) + '\n')
    print(len(rows) - 3, 'names traced', ('; no font for ' + ', '.join(missing)) if missing else '')


main()
