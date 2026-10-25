"""Sunstreak -- the user manual, built out of the game and its data (after Totality's Tools/manual).

    powershell -File tools/manual/make_shots.ps1      -> docs/manual/shots/*.png (the game takes its own pictures)
    py -3 tools/manual/make_manual.py                 -> docs/manual/Sunstreak-Manual.html (and .pdf)

The prose is `chapters.txt`, in a small markup: `# chapter`, `## section`, paragraphs separated by blank lines,
`- ` list items, `|` table rows (the first is the head, a `|---|` line is skipped), `[shot NAME] caption` for a picture
of docs/manual/shots, `[table NAME]` for a table made here from the data, `[stat NAME]` for a number, `**bold**` and
`code` in backticks. The tables of the legendary roads, the cars, the awards, the festivals and the command line are
read from the data files, the sources and the README, so the manual does not fall behind them.

The PDF is printed by Edge or Chrome in headless mode; without a browser the HTML is still written and prints by hand.
"""
import base64
import glob
import html
import io
import os
import re
import subprocess
import sys
import time

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
OUT = os.path.join(ROOT, 'docs', 'manual')

CSS = """
@page { size: A4; margin: 16mm 15mm 15mm 15mm; }
body { font: 10.5pt/1.55 "Segoe UI", "Helvetica Neue", Arial, sans-serif; color: #17181c; background: #fff; margin: 0; }
h1 { font-size: 34pt; margin: 0 0 2mm 0; letter-spacing: 2pt; color: #e0601a; font-style: italic; }
h2 { font-size: 17pt; margin: 0 0 3mm 0; padding-bottom: 2mm; border-bottom: 2px solid #f0a040; color: #b8460e; }
h3 { font-size: 11pt; margin: 5mm 0 1.5mm 0; color: #b8460e; letter-spacing: 0.6pt; text-transform: uppercase; }
p { margin: 0 0 3.2mm 0; text-align: justify; hyphens: auto; }
ul { margin: 0 0 3.2mm 0; padding-left: 6mm; list-style: none; }
li { margin: 0.8mm 0; }
ul > li { position: relative; }
ul > li::before { content: ''; position: absolute; left: -4mm; top: 0.62em; width: 1.5mm; height: 1.5mm; border-radius: 50%; background: #e0601a; }
code { font: 9.5pt Consolas, monospace; background: #f6f1ea; padding: 0 1mm; border-radius: 2px; }
.cover { page-break-after: always; text-align: center; padding-top: 18mm; }
.cover img.logo { width: 30mm; display: block; margin: 0 auto 5mm auto; }
.cover img.hero { width: 100%; border-radius: 4px; margin-top: 8mm; }
.cover p { text-align: center; }
.sub { color: #5b5f68; font-size: 13pt; margin: 0; }
.facts { margin-top: 7mm; color: #5b5f68; font-size: 9.5pt; }
.toc { page-break-after: always; }
.toc li { margin: 1.4mm 0; }
.chapter { page-break-before: always; }
figure { margin: 2mm 0 5mm 0; page-break-inside: avoid; }
figure img { width: 100%; border-radius: 3px; }
figcaption { color: #5b5f68; font-size: 9pt; margin-top: 1.5mm; font-style: italic; }
table { border-collapse: collapse; width: 100%; font-size: 9pt; margin: 2mm 0 5mm 0; page-break-inside: auto; }
tr { page-break-inside: avoid; }
th { text-align: left; color: #5b5f68; font-weight: 600; border-bottom: 1.5px solid #e0a060; padding: 1mm 1.5mm; }
td { border-bottom: 1px solid #efe8dd; padding: 0.9mm 1.5mm; vertical-align: top; }
td.mono { font-family: Consolas, monospace; font-size: 8.4pt; white-space: nowrap; }
"""

CONTROLS = [
    ('Accelerate', 'Up / W', 'RT or A'), ('Brake', 'Down / S', 'LT or X'), ('Steer', 'Left, Right / A, D', 'left stick, d-pad'),
    ('Boost', 'Space / Shift', 'B'), ('Gear down / up (manual gears)', 'Q / E', 'LB / RB'),
    ('Camera: chase, far, bumper, cockpit', 'C', 'Y'), ('Look back (hold)', 'B', 'right stick pressed in'),
    ('Radio station / next track', 'M / N', 'View'), ('Pause', 'Esc / P', 'Start'), ('Window / fullscreen', 'F11 or Alt+Enter', ''),
    ('Performance overlay', 'F3', ''), ('Read the data files again', 'F5', ''), ('Landmark viewer', 'F6', ''),
]

SETTINGS = [
    ('PROFILE', 'BATTERY, BALANCED or QUALITY: resolution, shadows and frame rate in one go'),
    ('DISPLAY', 'a window or fullscreen'), ('3D RESOLUTION', 'the 3D picture at 50 to 200 % of the window; the text stays sharp'),
    ('SHADOWS', 'the sun\'s shadows'), ('VSYNC', 'wait for the display'), ('FRAME LIMIT', 'a cap on the frame rate'),
    ('DIFFICULTY', 'EASY, NORMAL or HARD: grip, clock, traffic and rivals'),
    ('SCENERY', 'the world tour, the RANDOM TOUR to every country in random leaps, or ALL MIXED: every stage another biome'),
    ('TIME OF DAY', 'each place\'s own light, the TOUR CLOCK, or DAWN, DAY, DUSK, NIGHT'),
    ('WEATHER', 'weather fronts passing, or each place\'s own weather'),
    ('SEASONS', 'AS TODAY, AS YOU TRAVEL (a year every 400 km) or OFF'), ('POLICE', 'patrol cars that give chase'),
    ('STEER ASSIST', 'OFF, LIGHT or STRONG'), ('BRAKE ASSIST', 'brakes for bends and obstacles taken too fast'),
    ('COLOUR BLIND', 'the screen\'s red and green become vermilion and sky blue'), ('TEXT SIZE', '100, 115 or 130 %'),
    ('GEARS', 'automatic or manual'), ('UNITS', 'km/h or mph'), ('CAMERA', 'chase, far, bumper or cockpit'),
    ('BLUR', 'the speed blur'), ('VIBRATION', 'how strongly the gamepad rumbles'), ('MUSIC', 'the music\'s volume'),
    ('EFFECTS', 'the effects\' volume'), ('RADIO', 'the station, or AUTO'),
    ('RESET SETTINGS', 'every setting back to its default (press twice)'),
    ('ERASE PROGRESS', 'forget everything driven: records, places, unlocks, postcards, ghosts (press twice)'),
]

UNLOCKS = {'ARROW GT': 'from the start', 'SPARROW': 'after 30 km', 'VORTEX': 'after 100 km or a title',
           'KESTREL': 'after 40 countries'}
CAR_TEXT = {'ARROW GT': 'the all-rounder, a V8', 'SPARROW': 'light and revvy, a four-cylinder',
            'VORTEX': 'a V12, the fastest', 'KESTREL': 'a turbocharged rally hatchback, grippy on gravel and sand'}


def read(path):
    return open(os.path.join(ROOT, path), encoding='utf-8').read()


def places():
    """Every place of data/places with its steps: (id, name, country, region, {step: [rest, ...]})."""
    out = []
    for path in sorted(glob.glob(os.path.join(ROOT, 'data', 'places', '*.txt'))):
        cur = None
        for line in open(path, encoding='utf-8'):
            if line.startswith('place '):
                p = [s.strip() for s in line.split('|')]
                cur = (p[0].split()[1], p[1], p[2] if len(p) > 2 else '', p[4] if len(p) > 4 else '', {})
                out.append(cur)
            elif cur and line.startswith('    ') and line.strip() and not line.strip().startswith('#'):
                words = line.strip().split(' ', 1)
                cur[4].setdefault(words[0], []).append(words[1] if len(words) > 1 else '')
    return out


def stats():
    route = [l.split() for l in read('data/route.txt').splitlines() if l.strip() and not l.startswith('#')]
    by_id = {p[0]: p for p in places()}
    # The first places are built in (src/WorldBiomes.cpp), and the route runs through them too.
    for name, country in re.findall(r'\{ BiomeT::(\w+), "([^"]+)"', read('src/WorldBiomes.cpp')):
        by_id.setdefault(name, (name, name, country, '', {}))
    countries = {by_id[r[0]][2] for r in route if r[0] in by_id and by_id[r[0]][2] not in ('', '-')}
    legends = sum(1 for p in by_id.values() if 'legend' in p[4])
    words = {20: 'Twenty', 21: 'Twenty-one', 22: 'Twenty-two', 23: 'Twenty-three', 24: 'Twenty-four', 25: 'Twenty-five',
             26: 'Twenty-six', 27: 'Twenty-seven', 28: 'Twenty-eight', 29: 'Twenty-nine', 30: 'Thirty'}
    return {'places': str(len(route)), 'countries': str(len(countries)), 'legends': words.get(legends, str(legends))}


# The game writes in capitals; the manual sets names in title case and texts as sentences.
ACRONYMS = {'USA', 'UK', 'UAE', 'DR', 'GT', 'FM', 'KM', 'M'}
SMALL = {'a', 'an', 'and', 'at', 'da', 'de', 'del', 'della', 'dello', 'des', 'di', 'du', 'for', 'in', 'la', 'le', 'na', 'of',
         'on', 'the', 'to', 'van', 'von', 'y'}
PROPER = ["HEAVEN'S GATE", 'SOUTHERN OCEAN', 'ARCTIC OCEAN', 'MONTE CARLO RALLY', 'GREEN HELL', 'RHONE GLACIER',
          'FAGARAS MOUNTAINS', 'CONTINENTAL DIVIDE', 'DEATH ROAD', 'MOTHER ROAD', 'BIG SUR', 'HIGHWAY 1', 'KHUNJERAB PASS',
          'GATA LOOPS', 'IROHA', 'HOCHTOR', 'STIGFOSSEN', 'EARTH', 'EUROPE', 'AFRICA', 'ALASKA', 'TIERRA DEL FUEGO',
          'SOUTH, SOUTHEAST AND EAST ASIA']


def title(text):
    words = []
    parts = text.split(' ')
    for i, w in enumerate(parts):
        if w in ACRONYMS:
            words.append(w)
        elif 0 < i < len(parts) - 1 and w.lower() in SMALL:
            words.append(w.lower())
        else:
            words.append('-'.join('/'.join(x[:1].upper() + x[1:].lower() for x in part.split('/')) for part in w.split('-')))
    return ' '.join(words)


def sentence(text):
    s = text.lower()
    names = PROPER + sorted({p[2] for p in places() if len(p[2]) > 2}, key=len, reverse=True)
    for name in names:
        s = re.sub(r'\b' + re.escape(name.lower()) + r'\b', lambda m, n=title(name): n, s)
    return s[:1].upper() + s[1:]


def table(head, rows, mono=()):
    out = ['<table><tr>' + ''.join(f'<th>{html.escape(h)}</th>' for h in head) + '</tr>']
    for r in rows:
        out.append('<tr>' + ''.join(f'<td class="mono">{html.escape(str(c))}</td>' if i in mono else f'<td>{inline(str(c))}</td>'
                                    for i, c in enumerate(r)) + '</tr>')
    return '\n'.join(out) + '</table>'


def table_legends():
    rows = []
    for pid, name, country, region, steps in places():
        if 'legend' not in steps:
            continue
        hp = steps.get('serpentine', [''])[0].split(' ', 1)
        pins = hp[0] if hp[0] else ''
        summit = steps.get('summit', [''])[0].split(' ', 1)
        top = f'{title(summit[1])} {summit[0]} m' if len(summit) == 2 else ''
        rows.append((title(name), title(country), sentence(steps['legend'][0]), pins, top))
    rows.sort(key=lambda r: r[0])
    return table(('Road', 'Country', 'What it is', 'Hairpins', 'Top'), rows)


def table_cars():
    src = read('src/Models.cpp')
    rows = []
    for m in re.finditer(r'\{\s*"([A-Z ]+)",\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*([0-9.]+)f,\s*(\d+),', src):
        name, top, cyl = m.group(1), float(m.group(2)), int(m.group(7))
        rows.append((name, CAR_TEXT.get(name, ''), f'{top * 3.6:.0f} km/h', f'{cyl} cylinders', UNLOCKS.get(name, '')))
    return table(('Car', 'What it is', 'Top speed', 'Engine', 'Yours'), rows)


def table_awards():
    src = read('src/Achievements.cpp')
    rows = [(title(m.group(2)), sentence(m.group(3))) for m in re.finditer(r'\{\s*"(\w+)",\s*"([^"]+)",\s*"([^"]+)"\s*\}', src)]
    return table(('Award', 'How'), rows)


def table_festivals():
    rows = []
    for line in read('data/festivals.txt').splitlines():
        if line.startswith('festival '):
            p = [s.strip() for s in line[9:].split('|')]
            dates = p[2].split()
            when = ' to '.join('{}.{}.'.format(*reversed(d.split('-'))) for d in dates)
            where = p[3] if len(p) > 3 else ''
            if len(where) > 70:
                where = where[:where.rfind(',', 0, 70)] + ' and more'
            lat = re.fullmatch(r'(north|south) (\d+)', where)
            where = 'Everywhere' if where == '*' else f'{lat[1].title()} of {lat[2]}°' if lat else title(where)
            rows.append((sentence(p[1]), when, where))
    return table(('Festival', 'When', 'Where'), rows)


def table_options():
    text = read('README.md')
    block = text[text.index('## Settings and command line'):text.index('## Code')]
    rows = []
    for line in block.splitlines():
        if line.startswith('    --'):
            m = re.match(r'    (--\S.*?)\s{2,}(.*)$', line)
            if m:
                rows.append([m.group(1).strip(), m.group(2).strip()])
            else:
                rows.append([line.strip(), ''])
        elif line.startswith('     ') and rows:
            rows[-1][1] = (rows[-1][1] + ' ' + line.strip()).strip()
    return table(('Option', 'What it does'), rows, mono=(0,))


TABLES = {
    'controls': lambda: table(('', 'Keyboard', 'Gamepad, Steam Deck'), CONTROLS),
    'settings': lambda: table(('Setting', 'What it does'), SETTINGS),
    'legends': table_legends, 'cars': table_cars, 'awards': table_awards, 'festivals': table_festivals, 'options': table_options,
}


def inline(text, stat=None):
    t = html.escape(text)
    t = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', t)
    t = re.sub(r'`([^`]+)`', r'<code>\1</code>', t)
    if stat:
        t = re.sub(r'\[stat (\w+)\]', lambda m: stat.get(m.group(1), m.group(0)), t)
    return t


def picture(name, width=1400):
    path = os.path.join(OUT, 'shots', name + '.png')
    if not os.path.exists(path):
        print('missing picture', name)
        return ''
    im = Image.open(path).convert('RGB')
    if im.width > width:
        im = im.resize((width, round(im.height * width / im.width)), Image.LANCZOS)
    buf = io.BytesIO()
    im.save(buf, 'JPEG', quality=84, optimize=True)
    return 'data:image/jpeg;base64,' + base64.b64encode(buf.getvalue()).decode('ascii')


def logo():
    src = read('src/IconPng.h')
    data = bytes(int(v) for v in re.findall(r'\b\d+\b', src[src.index('{', src.index('IconPng[]')):]))
    return 'data:image/png;base64,' + base64.b64encode(data).decode('ascii')


def version():
    m = re.search(r'project\(\s*Sunstreak\s+VERSION\s+([0-9.]+)', read('CMakeLists.txt'))
    return m.group(1) if m else ''


def render(chapters, stat):
    body, toc = [], []
    blocks = re.split(r'\n\s*\n', chapters.strip())
    open_chapter = False
    for block in blocks:
        lines = block.strip().split('\n')
        first = lines[0]
        if first.startswith('# '):
            if open_chapter:
                body.append('</div>')
            title = first[2:].strip()
            anchor = re.sub(r'\W+', '-', title.lower()).strip('-')
            toc.append(f'<li><a href="#{anchor}">{html.escape(title)}</a></li>')
            body.append(f'<div class="chapter" id="{anchor}"><h2>{html.escape(title)}</h2>')
            open_chapter = True
            lines = lines[1:]
            if not lines:
                continue
            first = lines[0]
        if first.startswith('## '):
            body.append(f'<h3>{html.escape(first[3:].strip())}</h3>')
            lines = lines[1:]
            if not lines:
                continue
            first = lines[0]
        if first.startswith('[shot '):
            for line in lines:
                m = re.match(r'\[shot (\w+)\]\s*(.*)', line)
                if m:
                    src = picture(m.group(1))
                    if src:
                        body.append(f'<figure><img src="{src}" alt="{html.escape(m.group(2))}"><figcaption>{inline(m.group(2), stat)}</figcaption></figure>')
        elif first.startswith('[table '):
            body.append(TABLES[first[7:-1].strip()]())
        elif first.startswith('|'):
            rows = [[c.strip() for c in l.strip().strip('|').split('|')] for l in lines if not re.match(r'\|\s*-', l)]
            body.append(table(rows[0], rows[1:]))
        elif first.startswith('- '):
            items, cur = [], None
            for line in lines:
                if line.startswith('- '):
                    cur = line[2:]
                    items.append(cur)
                else:
                    items[-1] += ' ' + line.strip()
            body.append('<ul>' + ''.join(f'<li>{inline(i, stat)}</li>' for i in items) + '</ul>')
        else:
            body.append(f'<p>{inline(" ".join(l.strip() for l in lines), stat)}</p>')
    if open_chapter:
        body.append('</div>')
    return toc, body


def find_browser():
    for p in (r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe', r'C:\Program Files\Microsoft\Edge\Application\msedge.exe',
              r'C:\Program Files\Google\Chrome\Application\chrome.exe'):
        if os.path.exists(p):
            return p
    return None


def main():
    stat = stats()
    toc, body = render(read('tools/manual/chapters.txt'), stat)
    ver = version()
    cover = f"""<div class="cover"><img class="logo" src="{logo()}" alt="">
<h1>SUNSTREAK</h1><p class="sub">The manual &middot; version {ver}</p>
<img class="hero" src="{picture('cover', 1600)}" alt="">
<p class="facts">{stat['places']} places &middot; {stat['countries']} countries and territories &middot; {stat['legends'].lower()} legendary roads
&middot; thirty-one radio stations</p></div>
<div class="toc"><h2>Contents</h2><ol>{''.join(toc)}</ol></div>"""
    page = f"""<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"><title>Sunstreak Manual</title><style>{CSS}</style></head>
<body>{cover}{''.join(body)}</body></html>"""
    os.makedirs(OUT, exist_ok=True)
    out_html = os.path.join(OUT, 'Sunstreak-Manual.html')
    open(out_html, 'w', encoding='utf-8', newline='\n').write(page)
    print('wrote', out_html, f'{os.path.getsize(out_html) // 1024} KB')
    browser = find_browser()
    if not browser:
        print('no browser: print the HTML by hand')
        return
    pdf = os.path.join(OUT, 'Sunstreak-Manual.pdf')
    if os.path.exists(pdf):
        os.remove(pdf)
    url = 'file:///' + out_html.replace('\\', '/')
    # A profile of its own, or a browser already running takes the call over and prints nothing.
    profile = os.path.join(ROOT, 'work', 'edge-profile')
    subprocess.run([browser, '--headless=new', '--disable-gpu', '--no-pdf-header-footer', '--user-data-dir=' + profile,
                    '--print-to-pdf=' + pdf, url], check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
    # The launcher returns before its child has printed: wait until the file stops growing.
    size = -1
    for _ in range(240):
        time.sleep(0.5)
        now = os.path.getsize(pdf) if os.path.exists(pdf) else -1
        if now > 0 and now == size:
            break
        size = now
    print('wrote', pdf, f'{os.path.getsize(pdf) // 1024} KB' if os.path.exists(pdf) else '(failed)')


main()
