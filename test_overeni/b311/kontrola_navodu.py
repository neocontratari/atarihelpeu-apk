#!/usr/bin/env python3
# B311: KONTROLA NAVODU PROTI APPCE - kazdy nazev tlacitka v navodu (assets/napoveda/index.html,
# <span class="t">) musi v appce opravdu byt, PRESNE tak, jak ho clovek uvidi (i s / bez hacku):
# hleda se ve strankach appky (assets/**/*.html, *.js, menu/karty.txt), v Jave (retezce, i \\uXXXX)
# a v C++ (texty kreslene v C++ - menu, pristroj Atari, intro). Co se nenajde, je vymysleny nazev.
# Nekolik "tlacitek" nejsou texty, ale kreslene znacky (kolecko, symboly PS1) - ty jsou
# vyjmenovane nize i s tim, kde v appce jsou; zkontroluje se aspon, ze ta znacka v kodu je.
#   python3 kontrola_navodu.py            -> VYSTUP_kontrola_navodu.txt
import glob
import html
import html.parser
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../../app/src/main'))
NAVOD = os.path.join(R, 'assets/napoveda/index.html')


class Cipy(html.parser.HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.hloubka = 0
        self.akt = None
        self.cipy = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
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


p = Cipy()
p.feed(open(NAVOD, encoding='utf-8').read())
cipy = sorted(set(p.cipy))


def java_text(s):
    # retezce Javy: \uXXXX -> znak
    return re.sub(r'\\u([0-9a-fA-F]{4})', lambda m: chr(int(m.group(1), 16)), s)


korpus = []
for f in glob.glob(R + '/assets/**/*', recursive=True):
    if os.path.isdir(f) or 'napoveda' in f:
        continue
    if f.endswith(('.html', '.js', '.txt')):
        t = open(f, encoding='utf-8', errors='replace').read()
        korpus.append((os.path.relpath(f, R), html.unescape(t)))
for f in glob.glob(R + '/java/**/*.java', recursive=True):
    korpus.append((os.path.relpath(f, R), java_text(open(f, encoding='utf-8', errors='replace').read())))
for f in glob.glob(R + '/cpp/**/*', recursive=True):
    if '/vendor/' in f or os.path.isdir(f) or not f.endswith(('.h', '.cpp')):
        continue
    t = open(f, encoding='utf-8', errors='replace').read()
    # C++: \xE2\x86\x90 apod. (UTF-8 po bajtech) -> znak
    def utf8(m):
        try:
            return bytes(int(x, 16) for x in re.findall(r'\\x([0-9a-fA-F]{2})', m.group(0))).decode('utf-8')
        except Exception:
            return m.group(0)
    t = re.sub(r'(?:\\x[0-9a-fA-F]{2})+', utf8, t)
    korpus.append((os.path.relpath(f, R), t))

# kreslene znacky (ne text) - kde jsou v appce
ZNACKY = {
    '⚙': ('kolecko OPTIONS / D-PAD A OVLADANI', ['assets/index.html', 'assets/emu_sega/index.html', 'assets/emu_ps1/index.html']),
    '△': ('PS1 trojuhelnik', ['assets/emu_ps1/index.html']),
    '○': ('PS1 kolecko', ['assets/emu_ps1/index.html']),
    '✕': ('PS1 krizek', ['assets/emu_ps1/index.html']),
    '□': ('PS1 ctverec', ['assets/emu_ps1/index.html']),
}
ZNACKY_ENT = {'⚙': ['&#9881;', '⚙'], '△': ['△', '&#9651;', '\\u25b3', 'triangle', 'TROJ'],
              '○': ['○', '&#9675;', 'circle', 'KOLE'], '✕': ['✕', '&#10005;', 'cross', 'KRIZ', 'KŘÍŽ'],
              '□': ['□', '&#9633;', 'square', 'CTVER', 'ČTVER']}

vystup = []
chyb = 0


def tisk(s):
    print(s)
    vystup.append(s)


tisk('Navod: %d ruznych nazvu tlacitek (<span class="t">)' % len(cipy))
for c in cipy:
    if c in ZNACKY:
        popis, soubory = ZNACKY[c]
        ok = False
        for f in soubory:
            t = open(os.path.join(R, f), encoding='utf-8', errors='replace').read()
            if any(v in t for v in ZNACKY_ENT[c]):
                ok = True
                break
        tisk('%s  %-34s kreslena znacka (%s)' % ('OK   ' if ok else 'CHYBA', c, popis))
        chyb += 0 if ok else 1
        continue
    kde = [jm for jm, t in korpus if c in t]
    if kde:
        tisk('OK     %-34s %s' % (c, ', '.join(kde[:3]) + (' ...' if len(kde) > 3 else '')))
    else:
        # cast slozeneho nazvu (napr. "USB 1 ▾") - nazev bez sipky / bez mezer kolem lomitka
        alt = [c.replace(' / ', '/'), c.replace('/', ' / '), c.replace(' ▾', ''), c.replace('◇ ', '')]
        kde2 = [jm for jm, t in korpus for a in alt if a != c and a in t]
        if kde2:
            tisk('OK     %-34s (jako "%s") %s' % (c, next(a for a in alt if a != c and any(a in t for _, t in korpus)), kde2[0]))
        else:
            chyb += 1
            tisk('CHYBA  %-34s V APPCE NENI' % c)

tisk('VYSLEDEK: %d nazvu, %d CHYBA (nazev v navodu, ktery v appce neni)' % (len(cipy), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_navodu.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
