#!/usr/bin/env python3
# B316: NAVOD V APPCE (assets/napoveda/index.html a en.html) - co presne se zmenilo proti B315.
#  Kazdy blok textu (nadpis, odstavec, polozka, otazka, dt / dd ...) z B315 (git 0fd67c7) se porovna
#  s B316. Smi se zmenit JEN dva bloky v kazdem jazyce:
#   - XEX / MOBIL (XEX / PHONE): pribyla cartridge CAR, VYSUNOUT CARTRIDGE, VYBRAT JINY SOUBOR
#   - NET / HRY: "hra pro Atari (XEX, ATR, CAR, ZIP)"
#  Zadny blok nepribyl ani nezmizel, znacky HTML (kapitoly, odkazy, obrazky) stejne.
#   python3 kontrola_navodu_b316.py      -> VYSTUP_kontrola_navodu_b316.txt
import difflib
import html.parser
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
B315 = '0fd67c7'
BLOKY = {'h1', 'h2', 'h3', 'h4', 'p', 'li', 'dt', 'dd', 'summary', 'figcaption', 'th', 'td', 'title', 'button', 'label'}
vystup = []
chyb = 0


def tisk(s):
    print(s)
    vystup.append(s)


class Bloky(html.parser.HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.zasobnik = []
        self.bloky = []
        self.znacky = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        self.znacky.append((tag, a.get('id'), a.get('href'), a.get('src'), a.get('class')))
        if tag in BLOKY:
            self.zasobnik.append([tag, []])

    def handle_endtag(self, tag):
        if tag in BLOKY and self.zasobnik and self.zasobnik[-1][0] == tag:
            t, cast = self.zasobnik.pop()
            text = re.sub(r'\s+', ' ', ''.join(cast)).strip()
            if text:
                self.bloky.append('%s: %s' % (t, text))
            if self.zasobnik:
                self.zasobnik[-1][1].append(' ' + ''.join(cast) + ' ')

    def handle_data(self, d):
        if self.zasobnik:
            self.zasobnik[-1][1].append(d)


def rozbor(text):
    p = Bloky()
    p.feed(text)
    return p


def kontrola(soubor, ocekavane):
    global chyb
    stary = subprocess.run(['git', 'show', '%s:app/src/main/assets/napoveda/%s' % (B315, soubor)], cwd=KOREN,
                           capture_output=True, text=True, check=True).stdout
    novy = open(os.path.join(KOREN, 'app/src/main/assets/napoveda', soubor), encoding='utf-8').read()
    a, b = rozbor(stary), rozbor(novy)
    zmeny = []
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a.bloky, b.bloky, autojunk=False).get_opcodes():
        if op != 'equal':
            zmeny.append((op, a.bloky[i1:i2], b.bloky[j1:j2]))
    tisk('%s: %d bloku textu v B315, %d v B316' % (soubor, len(a.bloky), len(b.bloky)))
    zmeneno = 0
    for op, st, nv in zmeny:
        for x in st:
            tisk('   B315  %s' % x[:300])
        for x in nv:
            tisk('   B316  %s' % x[:300])
        zmeneno += max(len(st), len(nv))
        ok = op == 'replace' and len(st) == len(nv) and all(any(k in x for k in ocekavane) for x in nv)
        if not ok:
            chyb += 1
            tisk('CHYBA  necekana zmena (%s)' % op)
    if zmeneno == 2:
        tisk('OK     %s: zmenene presne 2 bloky (XEX / MOBIL a NET / HRY), %d ostatnich beze zmeny' % (soubor, len(b.bloky) - 2))
    else:
        chyb += 1
        tisk('CHYBA  %s: zmenenych bloku %d (ma byt 2)' % (soubor, zmeneno))
    # znacky: smi pribyt jen 2x <span class="t"> (nazvy tlacitek VYSUNOUT CARTRIDGE / VYBRAT JINY SOUBOR)
    jine = []
    pribylo = 0
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a.znacky, b.znacky, autojunk=False).get_opcodes():
        if op == 'equal':
            continue
        if op == 'insert' and all(z == ('span', None, None, None, 't') for z in b.znacky[j1:j2]):
            pribylo += j2 - j1
        else:
            jine.append((op, a.znacky[i1:i2], b.znacky[j1:j2]))
    if not jine and pribylo == 2:
        tisk('OK     %s: znacky HTML (kapitoly, odkazy, obrazky, tridy) beze zmeny, jen 2x pribyl nazev tlacitka '
             '<span class="t"> (%d znacek)' % (soubor, len(b.znacky)))
    else:
        chyb += 1
        tisk('CHYBA  %s: znacky HTML - pribylo nazvu tlacitek %d, jinych zmen %d: %s' % (soubor, pribylo, len(jine), jine[:3]))


tisk('B316 - navod v appce proti B315 (git %s): smi se zmenit jen XEX / MOBIL a NET / HRY (pribyla cartridge CAR)' % B315)
kontrola('index.html', ['cartridge CAR', 'XEX, ATR, CAR, ZIP'])
kontrola('en.html', ['CAR cartridge', 'XEX, ATR, CAR, ZIP'])
tisk('VYSLEDEK: %d CHYBA' % chyb)
open(os.path.join(ZDE, 'VYSTUP_kontrola_navodu_b316.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
