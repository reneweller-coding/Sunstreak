"""Draws Sunstreak's icon: a road running into a striped sun setting on the horizon, the red car on its way.

Writes res/sunstreak.ico (16 to 256 px, the Windows program icon), res/sunstreak.png (512 px) and
src/IconPng.h (a 128 px PNG as bytes: the window's icon on every platform).

    python tools/make_icon.py
"""
import io
import os

from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
S = 2048                # drawn this large, then scaled down smoothly
HORIZON = int(S * 0.6)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def gradient(draw, box, stops):
    """A vertical gradient through (t, colour) stops inside box."""
    x0, y0, x1, y1 = box
    for y in range(y0, y1):
        t = (y - y0) / max(1, y1 - y0 - 1)
        for k in range(len(stops) - 1):
            if stops[k][0] <= t <= stops[k + 1][0]:
                u = (t - stops[k][0]) / (stops[k + 1][0] - stops[k][0])
                draw.line([(x0, y), (x1, y)], fill=lerp(stops[k][1], stops[k + 1][1], u))
                break


def icon():
    img = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    scene = Image.new('RGBA', (S, S))
    d = ImageDraw.Draw(scene)

    # The sky: night above, the glow of the sunset at the horizon.
    gradient(d, (0, 0, S, HORIZON), [(0.0, (24, 16, 64)), (0.55, (120, 40, 130)), (1.0, (255, 120, 70))])

    # The sun on the horizon, yellow to pink, cut by bands that widen towards the ground.
    sun = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    sd = ImageDraw.Draw(sun)
    r, cx, cy = int(S * 0.3), S // 2, HORIZON
    gradient(sd, (cx - r, cy - r, cx + r, cy), [(0.0, (255, 236, 110)), (1.0, (255, 80, 120))])
    mask = Image.new('L', (S, S), 0)
    md = ImageDraw.Draw(mask)
    md.ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
    y, gap = cy - int(r * 0.42), int(S * 0.008)
    while y < cy:
        md.rectangle((0, y, S, y + gap), fill=0)
        y += int(S * 0.045)
        gap = int(gap * 1.35)
    glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(glow).ellipse((cx - r * 1.25, cy - r * 1.25, cx + r * 1.25, cy + r * 1.25), fill=(255, 140, 90, 110))
    scene.alpha_composite(glow.filter(ImageFilter.GaussianBlur(S * 0.04)))
    scene.paste(sun, (0, 0), mask)

    # The land, dark, and the road running to the sun.
    gradient(d, (0, HORIZON, S, S), [(0.0, (40, 20, 60)), (1.0, (14, 10, 28))])
    road = [(cx - S * 0.02, HORIZON), (cx + S * 0.02, HORIZON), (cx + S * 0.48, S), (cx - S * 0.48, S)]
    d.polygon(road, fill=(58, 52, 78))
    for side in (-1, 1):
        d.line([(cx + side * S * 0.022, HORIZON), (cx + side * S * 0.43, S)], fill=(240, 236, 250), width=int(S * 0.012))
    # The yellow middle: dashes shrinking towards the horizon.
    z = 0.0
    while True:
        t0, t1 = 0.06 + z, 0.06 + z + 0.07
        if t0 > 1.0:
            break
        ya, yb = HORIZON + (S - HORIZON) * t0 ** 1.6, HORIZON + (S - HORIZON) * min(1.0, t1) ** 1.6
        wa, wb = S * 0.004 + S * 0.02 * t0, S * 0.004 + S * 0.02 * min(1.0, t1)
        d.polygon([(cx - wa, ya), (cx + wa, ya), (cx + wb, yb), (cx - wb, yb)], fill=(255, 205, 50))
        z += 0.14

    # The car from behind, low and wide, its tail lights glowing.
    w, h, by = S * 0.36, S * 0.13, S * 0.9
    d.rounded_rectangle((cx - w / 2, by - h, cx + w / 2, by), radius=S * 0.03, fill=(214, 40, 48))
    d.rounded_rectangle((cx - w * 0.33, by - h * 1.75, cx + w * 0.33, by - h * 0.85), radius=S * 0.035, fill=(170, 26, 40))
    d.rounded_rectangle((cx - w * 0.27, by - h * 1.6, cx + w * 0.27, by - h * 1.0), radius=S * 0.02, fill=(40, 34, 70))
    for side in (-1, 1):
        x = cx + side * w * 0.33
        d.rounded_rectangle((x - w * 0.11, by - h * 0.72, x + w * 0.11, by - h * 0.42), radius=S * 0.01, fill=(255, 90, 80))
        d.rectangle((cx + side * w * 0.46 - S * 0.03, by - S * 0.005, cx + side * w * 0.46 + S * 0.03, by + S * 0.03), fill=(20, 16, 30))
    lights = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    ld = ImageDraw.Draw(lights)
    for side in (-1, 1):
        x = cx + side * w * 0.33
        ld.ellipse((x - w * 0.2, by - h * 0.95, x + w * 0.2, by - h * 0.2), fill=(255, 60, 60, 120))
    scene.alpha_composite(lights.filter(ImageFilter.GaussianBlur(S * 0.02)))

    # A rounded tile.
    tile = Image.new('L', (S, S), 0)
    ImageDraw.Draw(tile).rounded_rectangle((0, 0, S - 1, S - 1), radius=int(S * 0.18), fill=255)
    img.paste(scene, (0, 0), tile)
    return img


def main():
    big = icon()
    os.makedirs(os.path.join(ROOT, 'res'), exist_ok=True)
    sizes = [16, 20, 24, 32, 40, 48, 64, 128, 256]
    big.resize((256, 256), Image.LANCZOS).save(os.path.join(ROOT, 'res', 'sunstreak.ico'), sizes=[(s, s) for s in sizes])
    big.resize((512, 512), Image.LANCZOS).save(os.path.join(ROOT, 'res', 'sunstreak.png'))
    buf = io.BytesIO()
    big.resize((128, 128), Image.LANCZOS).save(buf, 'PNG', optimize=True)
    data = buf.getvalue()
    lines = ['/**', ' * @file IconPng.h', " * @brief The program's icon as a 128 px PNG (tools/make_icon.py writes it).", ' * @author reneweller', ' */',
             '#pragma once', '', 'namespace sunstreak', '{', '',
             '/** @brief The PNG file. */', 'inline constexpr unsigned char IconPng[] = {']
    for i in range(0, len(data), 20):
        lines.append('    ' + ', '.join(str(b) for b in data[i:i + 20]) + ',')
    lines += ['};', '', '} // namespace sunstreak', '']
    open(os.path.join(ROOT, 'src', 'IconPng.h'), 'w', newline='\n').write('\n'.join(lines))
    print('icon:', len(data), 'bytes of PNG')


if __name__ == '__main__':
    main()
