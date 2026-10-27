#!/usr/bin/env python3
# B317: NAVOD V APPCE (assets/napoveda/index.html a en.html) - co presne se zmenilo proti B316.
#  Kazdy blok textu (nadpis, odstavec, polozka, otazka, dt / dd ...) z B316 (git d7c85b9) se porovna
#  s B317. Smi se zmenit jen:
#   - kapitola PS1, blok "Kolecko - D-PAD A OVLADANI": v seznamu tlacitek pribylo MYS PLAYSTATION (DUNE 2000 ...)
#   - kapitola PS1: NOVY blok "Mys PlayStation (Dune 2000 ...)" na konci kapitoly (nadpis, odstavec,
#     5 polozek, TIP)
#  Nic jineho nepribylo ani nezmizelo; znacky HTML mimo novy blok stejne (kapitoly, odkazy, obrazky).
#   python3 kontrola_navodu_b317.py      -> VYSTUP_kontrola_navodu_b317.txt
import difflib
import html.parser
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
B316 = 'd7c85b9'
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


def kontrola(soubor, dpad_novy, nadpis_bloku, mys_slovo):
    global chyb
    stary = subprocess.run(['git', 'show', '%s:app/src/main/assets/napoveda/%s' % (B316, soubor)], cwd=KOREN,
                           capture_output=True, text=True, check=True).stdout
    novy = open(os.path.join(KOREN, 'app/src/main/assets/napoveda', soubor), encoding='utf-8').read()
    a, b = rozbor(stary), rozbor(novy)
    tisk('%s: %d bloku textu v B316, %d v B317' % (soubor, len(a.bloky), len(b.bloky)))
    zmeny = [z for z in difflib.SequenceMatcher(None, a.bloky, b.bloky, autojunk=False).get_opcodes() if z[0] != 'equal']
    ok_dpad = ok_novy = False
    jine = 0
    for op, i1, i2, j1, j2 in zmeny:
        st, nv = a.bloky[i1:i2], b.bloky[j1:j2]
        for x in st:
            tisk('   B316  %s' % x[:260])
        for x in nv:
            tisk('   B317  %s' % x[:260])
        if op == 'replace' and len(st) == 1 and len(nv) == 1 and st[0].replace(' ', '') == nv[0].replace(dpad_novy, '').replace(' ', ''):
            ok_dpad = True
        elif op == 'insert' and nv and nv[0] == 'h3: ' + nadpis_bloku and len(nv) == 8 and \
                all(mys_slovo.lower() in x.lower() or x.startswith('li: ') for x in nv[:2]) and nv[-1].startswith('p: TIP'):
            ok_novy = True
        else:
            jine += 1
    if not ok_dpad:
        chyb += 1
    tisk('%s  %s: v D-PAD A OVLADANI (kapitola PS1) pribylo jen "%s"' % ('OK   ' if ok_dpad else 'CHYBA', soubor, dpad_novy.strip(', ')))
    if not ok_novy:
        chyb += 1
    tisk('%s  %s: novy blok "%s" (nadpis, odstavec, 5 polozek, TIP)' % ('OK   ' if ok_novy else 'CHYBA', soubor, nadpis_bloku))
    if jine:
        chyb += 1
    tisk('%s  %s: jine zmeny textu: %d' % ('OK   ' if not jine else 'CHYBA', soubor, jine))
    # kapitoly, odkazy a obrazky (novy blok zadne nema): stejne
    za = [z for z in a.znacky if z[0] in ('section', 'a', 'img')]
    zb = [z for z in b.znacky if z[0] in ('section', 'a', 'img')]
    ok = za == zb
    if not ok:
        chyb += 1
    tisk('%s  %s: kapitoly, odkazy a obrazky beze zmeny (%d)' % ('OK   ' if ok else 'CHYBA', soubor, len(zb)))
    # novy blok je v kapitole PS1 (pred kapitolou MP3)
    i_ps1, i_mp3, i_blok = novy.find('id="ps1"'), novy.find('id="mp3"'), novy.find(nadpis_bloku)
    ok = 0 <= i_ps1 < i_blok < i_mp3
    if not ok:
        chyb += 1
    tisk('%s  %s: novy blok je v kapitole PS1' % ('OK   ' if ok else 'CHYBA', soubor))


kontrola('index.html', ', MYŠ PLAYSTATION (DUNE 2000 …)', 'Myš PlayStation (Dune 2000 …)', 'myš')
kontrola('en.html', ', PLAYSTATION MOUSE (DUNE 2000 …)', 'PlayStation mouse (Dune 2000 …)', 'mouse')
tisk('VYSLEDEK: %d CHYBA' % chyb)
open(os.path.join(ZDE, 'VYSTUP_kontrola_navodu_b317.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
