"""Sunstreak -- the picture GitHub shows when the project is linked: docs/social-preview.png (1280 x 640).

    python tools/make_preview.py

Tokyo at night as the ground (docs/screenshots/tokyo-photo.jpg, from the game's photo mode, no HUD), darkened towards the left for the
logo, the name and what the game is; along the bottom three more places. GitHub takes it by hand: Settings, Social
preview. After the synthesizers' make_preview.py.

@author Rene Weller
"""
import os

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
W, H = 1280, 640
SHOTS = os.path.join(ROOT, 'docs', 'screenshots')


def font(name, size):
    """A Windows font, or PIL's own where it is missing."""
    try:
        return ImageFont.truetype(os.path.join('C:/Windows/Fonts', name), size)
    except OSError:
        return ImageFont.load_default()


def shot(name, w, h):
    """A screenshot cut to w x h round its middle."""
    im = Image.open(os.path.join(SHOTS, name)).convert('RGB')
    scale = max(w / im.width, h / im.height)
    im = im.resize((round(im.width * scale), round(im.height * scale)), Image.LANCZOS)
    x, y = (im.width - w) // 2, (im.height - h) // 2
    return im.crop((x, y, x + w, y + h))


img = shot('tokyo-photo.jpg', W, H)
# Night falls from the left: the text stands on near-black, the city shines on the right.
shade = Image.new('L', (W, H))
for x in range(W):
    a = 235 if x < 380 else max(0, int(235 - (x - 380) * 0.42))
    shade.paste(a, (x, 0, x + 1, H))
img = Image.composite(Image.new('RGB', (W, H), (6, 8, 16)), img, shade)
d = ImageDraw.Draw(img)

logo = Image.open(os.path.join(ROOT, 'res', 'sunstreak.png')).convert('RGBA').resize((112, 112), Image.LANCZOS)
img.paste(logo, (56, 50), logo)
d.text((52, 168), 'SUNSTREAK', font=font('seguibli.ttf', 74), fill=(255, 128, 40))
d.text((58, 266), 'An arcade racer round the whole world', font=font('segoeuib.ttf', 30), fill=(244, 238, 230))
y = 318
for line in ('781 places in 202 countries, each driving its own way,',
             'real heights from the Dead Sea to the Himalaya,',
             '28 legendary roads, seasons, festivals and weather,',
             '31 radio stations composed as you drive.'):
    d.text((58, y), line, font=font('segoeui.ttf', 22), fill=(205, 212, 226))
    y += 31
d.text((58, y + 14), 'Windows  \u00b7  Linux  \u00b7  Steam Deck  \u00b7  AGPL-3.0',
       font=font('segoeui.ttf', 18), fill=(150, 160, 180))

# Three more places along the bottom right, framed.
tw, th, gap = 184, 104, 12
x0, y0 = W - 3 * tw - 2 * gap - 30, H - th - 22
for k, name in enumerate(('grossglockner-hairpins.jpg', 'glass-tunnel.jpg', 'christmas-lights.jpg')):
    x = x0 + k * (tw + gap)
    img.paste(shot(name, tw, th), (x, y0))
    d.rectangle([x - 2, y0 - 2, x + tw + 1, y0 + th + 1], outline=(255, 150, 60), width=2)

out = os.path.join(ROOT, 'docs', 'social-preview.png')
img.save(out)
print('wrote', os.path.relpath(out, ROOT), img.size)
