#!/usr/bin/env python3
# B314: KONTROLA ANGLICKEHO NAVODU (assets/napoveda/en.html) PROTI APPCE
#  1) kazdy nazev tlacitka (<span class="t">) musi v appce v ANGLICTINE opravdu byt, presne tak,
#     jak ho clovek uvidi: hleda se v anglickych slovnicich stranek (assets/jazyk/en_*.js),
#     v anglickych sloupcich menu/karty.txt, v Jave (anglicke texty tx(...) / NapJazyk.t(...)
#     i texty, ktere jsou anglicky v obou jazycich), v C++ (menu, Atari, intro) a ve strankach,
#     ktere jsou anglicky uz cesky (DJ, prehravac, jmena tlacitek jako START);
#     navic se vypise, jestli ho test v Chromiu opravdu videl na anglicke strance (VYSTUP_texty_*_en.txt);
#  2) zadny nazev tlacitka nesmi byt cesky (krome JAZYK · LANGUAGE a ČEŠTINA - prepinac jazyka);
#  3) en.html a index.html maji stejne kapitoly (id, poradi), stejne odkazy na kapitoly,
#     stejne obrazky a stejny pocet bloku / otazek;
#  4) presmerovani: index.html -> en.html pri AHJAZYK.kod()=='en', en.html -> index.html pri 'cs'.
#   python3 kontrola_navodu_en.py            -> VYSTUP_kontrola_navodu_en.txt
import glob
import html
import html.parser
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../../app/src/main'))
EN = os.path.join(R, 'assets/napoveda/en.html')
CZ = os.path.join(R, 'assets/napoveda/index.html')
sys.path.insert(0, ZDE)
from jazyk_harness import DIAKRITIKA  # noqa: E402

vystup = []
chyb = 0


def tisk(s):
    print(s)
    vystup.append(s)


class Cipy(html.parser.HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.hloubka = 0
        self.akt = None
        self.cipy = []
        self.kapitoly = []
        self.skoky = []
        self.obrazky = []
        self.pocty = {}

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        tr = (a.get('class') or '').split()
        if tag == 'section' and 'kap' in tr:
            self.kapitoly.append(a.get('id'))
        if tag == 'a' and (a.get('href') or '').startswith('#'):
            self.skoky.append(a.get('href')[1:])
        if tag == 'img':
            self.obrazky.append(a.get('src'))
        for k in ('blok', 'tip', 'pozor', 'uvod'):
            if k in tr:
                self.pocty[k] = self.pocty.get(k, 0) + 1
        if tag in ('details', 'li', 'dt', 'kbd'):
            self.pocty[tag] = self.pocty.get(tag, 0) + 1
        if self.akt is not None:
            self.hloubka += 1
        elif tag == 'span' and a.get('class') == 't':
            self.akt = []
            self.hloubka = 0

    def handle_endtag(self, tag):
        if self.akt is None:
            return
        if self.hloubka == 0:
            self.cipy.append(re.sub(r'\s+', ' ', ''.join(self.akt)).strip())
            self.akt = None
        else:
            self.hloubka -= 1

    def handle_data(self, d):
        if self.akt is not None:
            self.akt.append(d)


def rozeber(cesta):
    p = Cipy()
    p.feed(open(cesta, encoding='utf-8').read())
    return p


en = rozeber(EN)
cz = rozeber(CZ)
cipy = sorted(set(en.cipy))


def java_text(s):
    return re.sub(r'\\u([0-9a-fA-F]{4})', lambda m: chr(int(m.group(1), 16)), s)


def cpp_text(t):
    def utf8(m):
        try:
            return bytes(int(x, 16) for x in re.findall(r'\\x([0-9a-fA-F]{2})', m.group(0))).decode('utf-8')
        except Exception:
            return m.group(0)
    return re.sub(r'(?:\\x[0-9a-fA-F]{2})+', utf8, t)


# ---------- jen retezce a texty, ne komentare ani jmena promennych ----------
def literaly(t, cpp=False):
    """Retezce v kodu (Java / C++ / JS): "..." a '...' (JS), komentare // a /* */ se preskoci."""
    v = []
    i, n = 0, len(t)
    while i < n:
        c = t[i]
        if c == '/' and i + 1 < n and t[i + 1] == '/':
            j = t.find('\n', i)
            i = n if j < 0 else j
            continue
        if c == '/' and i + 1 < n and t[i + 1] == '*':
            j = t.find('*/', i + 2)
            i = n if j < 0 else j + 2
            continue
        if cpp and c == 'R' and t.startswith('R"', i):
            m = re.match(r'R"([^(\s]*)\(', t[i:])
            if m:
                konec = ')' + m.group(1) + '"'
                j = t.find(konec, i + m.end())
                if j >= 0:
                    v.append(t[i + m.end():j])
                    i = j + len(konec)
                    continue
        if c == '"' or (c == "'" and not cpp) or c == '`':
            j = i + 1
            buf = []
            while j < n and t[j] != c:
                if t[j] == '\\' and j + 1 < n:
                    buf.append(t[j:j + 2])
                    j += 2
                    continue
                if t[j] == '\n' and c != '`':
                    break
                buf.append(t[j])
                j += 1
            v.append(''.join(buf))
            i = j + 1
            continue
        if c == "'" and cpp:   # znak v C++
            j = t.find("'", i + 1)
            i = n if j < 0 else j + 1
            continue
        i += 1
    return v


class TextyHtml(html.parser.HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.v = []
        self.skript = False
        self.buf = []

    def handle_starttag(self, tag, attrs):
        for k, h in attrs:
            if h and k in ('title', 'aria-label', 'placeholder', 'alt', 'value', 'data-tip', 'content'):
                self.v.append(h)
        self.skript = tag in ('script', 'style')
        self.buf = []

    def handle_endtag(self, tag):
        if tag == 'script' and self.buf:
            self.v.extend(html.unescape(x) for x in literaly(''.join(self.buf)))
        self.skript = False
        self.buf = []

    def handle_data(self, d):
        if self.skript:
            self.buf.append(d)
        elif d.strip():
            self.v.append(d)


def texty_html(t):
    p = TextyHtml()
    p.feed(t)
    return '\n'.join(p.v)


# ---------- korpus anglickych textu appky ----------
korpus = []
for f in sorted(glob.glob(R + '/assets/jazyk/en_*.js')):
    korpus.append((os.path.relpath(f, R), '\n'.join(java_text(x) for x in literaly(open(f, encoding='utf-8').read()))))
# anglicke sloupce karet (11-14)
kt = []
for r in open(R + '/assets/menu/karty.txt', encoding='utf-8'):
    r = r.strip()
    if not r or r.startswith('#'):
        continue
    p = [x.strip() for x in r.split('|')]
    kt += p[10:14]
korpus.append(('assets/menu/karty.txt (EN sloupce)', '\n'.join(kt)))
# stranky (to, co je anglicky uz v ceske verzi - DJ, prehravac, START ...), bez navodu
for f in sorted(glob.glob(R + '/assets/**/*', recursive=True)):
    if os.path.isdir(f) or '/napoveda/' in f or '/jazyk/' in f:
        continue
    t = open(f, encoding='utf-8', errors='replace').read()
    if f.endswith('.html'):
        korpus.append((os.path.relpath(f, R), texty_html(t)))
    elif f.endswith('.js'):
        korpus.append((os.path.relpath(f, R), '\n'.join(java_text(x) for x in literaly(t))))
for f in sorted(glob.glob(R + '/java/**/*.java', recursive=True)):
    korpus.append((os.path.relpath(f, R), '\n'.join(java_text(x) for x in literaly(open(f, encoding='utf-8', errors='replace').read()))))
for f in sorted(glob.glob(R + '/cpp/**/*', recursive=True)):
    if '/vendor/' in f or os.path.isdir(f) or not f.endswith(('.h', '.cpp')):
        continue
    korpus.append((os.path.relpath(f, R), '\n'.join(cpp_text(x) for x in literaly(open(f, encoding='utf-8', errors='replace').read(), cpp=True))))

# co test v Chromiu opravdu videl na anglickych strankach
zive = ''
for f in sorted(glob.glob(ZDE + '/VYSTUP_texty_*_en.txt')):
    zive += open(f, encoding='utf-8').read() + '\n'

ZNACKY = {'⚙', '△', '○', '✕', '□', '&#9881;'}
CESKE_POVOLENE = {'JAZYK · LANGUAGE', 'ČEŠTINA'}

tisk('== 1) nazvy tlacitek v en.html (%d ruznych) proti anglickym textum appky' % len(cipy))
for c in cipy:
    if c in ZNACKY:
        tisk('OK     %-40s kreslena znacka (jako v ceskem navodu)' % c)
        continue
    def je_v(a, t):
        # cele slovo / cely nazev (ne "BACK" v "BACKGROUND")
        return re.search(r'(?<![A-Za-z0-9])' + re.escape(a) + r'(?![A-Za-z0-9])', t) is not None
    kde = [jm for jm, t in korpus if je_v(c, t)]
    alt = []
    if not kde:
        alt = [c.replace(' / ', '/'), c.replace('/', ' / '), c.replace(' ▾', ''), c.replace('◇ ', ''),
               c.replace(' ›', ''), c.upper()]
        kde = [jm for jm, t in korpus for a in alt if a != c and je_v(a, t)]
    if kde:
        tisk('OK     %-40s %s%s' % (c, ', '.join(sorted(set(kde))[:3]) + (' ...' if len(set(kde)) > 3 else ''),
                                    '  [videno v Chromiu]' if c in zive else ''))
    else:
        chyb += 1
        tisk('CHYBA  %-40s V APPCE ANGLICKY NENI' % c)

tisk('')
tisk('== 2) zadny nazev tlacitka v en.html neni cesky (krome prepinace jazyka)')
ceske = [c for c in cipy if DIAKRITIKA.search(c) and c not in CESKE_POVOLENE]
for c in ceske:
    tisk('CHYBA  cesky nazev v anglickem navodu: %s' % c)
chyb += len(ceske)
tisk('OK     ceskych nazvu: %d (povoleno: %s)' % (len(ceske), ', '.join(sorted(CESKE_POVOLENE))) if not ceske else '')

tisk('')
tisk('== 3) stejna stavba en.html a index.html')
for co, a, b in (('kapitoly (id a poradi)', en.kapitoly, cz.kapitoly),
                 ('odkazy na kapitoly (#...)', sorted(set(en.skoky)), sorted(set(cz.skoky))),
                 ('obrazky', en.obrazky, cz.obrazky)):
    ok = a == b
    chyb += 0 if ok else 1
    tisk('%s  %-28s EN %s' % ('OK   ' if ok else 'CHYBA', co, a if ok else '%s\n       CZ %s' % (a, b)))
for k in sorted(set(en.pocty) | set(cz.pocty)):
    a, b = en.pocty.get(k, 0), cz.pocty.get(k, 0)
    ok = a == b
    chyb += 0 if ok else 1
    tisk('%s  pocet %-8s EN %d  CZ %d' % ('OK   ' if ok else 'CHYBA', k, a, b))
# kazdy odkaz na kapitolu vede na existujici id
ids = set(re.findall(r'id="([^"]+)"', open(EN, encoding='utf-8').read()))
spatne = [s for s in set(en.skoky) if s not in ids]
chyb += len(spatne)
tisk('%s  odkazy v en.html vedou na existujici id%s' % ('OK   ' if not spatne else 'CHYBA', '' if not spatne else ': ' + ', '.join(spatne)))

tisk('')
tisk('== 4) presmerovani podle jazyka appky')
te = open(EN, encoding='utf-8').read()
tc = open(CZ, encoding='utf-8').read()
for jm, t, ma in (('en.html', te, "if(k==='cs')location.replace('index.html'"), ('index.html', tc, "if(k==='en')location.replace('en.html'")):
    ok = ma in t and 'AHJAZYK.kod()' in t and t.index(ma) < t.index('<style>')
    chyb += 0 if ok else 1
    tisk('%s  %-11s presmeruje (pred CSS, bez historie): %s' % ('OK   ' if ok else 'CHYBA', jm, ma))
ok = '<html lang="en">' in te and '<title>N&amp;P VISION — Guide</title>' in te
chyb += 0 if ok else 1
tisk('%s  en.html: lang="en", titulek Guide' % ('OK   ' if ok else 'CHYBA'))

tisk('')
tisk('VYSLEDEK: %d nazvu tlacitek, %d CHYBA' % (len(cipy), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_navodu_en.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
