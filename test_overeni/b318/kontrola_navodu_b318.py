#!/usr/bin/env python3
# B318: NAVOD V APPCE (assets/napoveda/index.html a en.html) - co presne se zmenilo proti B317.
#  Kazdy blok textu (nadpis, odstavec, polozka, otazka, dt / dd ...) z B317 (git 2f8fff4) se porovna
#  s B318. Smi se zmenit jen:
#   - kapitola PS1, blok "Mys PlayStation": polozka o tahu (kurzor se rozjede plynule), NOVA polozka
#     o zrychleni, TIP (mys pocitace pres WEB TV + klavesnice jako drive)
#   - kapitola WEB TV, blok "V prohlizeci": NOVA polozka o mysi PS1 (klik do obrazu, Esc)
#  Nic jineho nepribylo ani nezmizelo; znacky HTML mimo tyto bloky stejne (kapitoly, odkazy, obrazky).
#   python3 kontrola_navodu_b318.py      -> VYSTUP_kontrola_navodu_b318.txt
import difflib
import html.parser
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
B317 = '2f8fff4'
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


# (soubor, stare bloky smi zmizet (zacatek), nove bloky smi pribyt (zacatek))
POVOLENO = {
    'index.html': (
        ('li: Táhni prstem po obrazu = pohyb kurzoru. Ťukni', 'p: TIP Na TV / PC s myší v portu 1:'),
        ('li: Táhni prstem po obrazu = pohyb kurzoru. Kurzor se rozjede plynule',
         'li: Zrychlení jako u myši počítače: pomalý tah = jemný přesný posun',
         'p: TIP Na TV / PC přes WEB TV: klikni do obrazu – kurzor hry pak ovládá myš počítače',
         'li: PS1 se zapnutou myší PlayStation: klikni do obrazu')),
    'en.html': (
        ('li: Drag your finger over the picture = cursor movement. Tap', 'p: TIP On a TV / PC with the mouse in port 1:'),
        ('li: Drag your finger over the picture = cursor movement. The cursor starts moving smoothly',
         'li: Acceleration like a computer mouse: a slow drag = fine precise movement',
         'p: TIP On a TV / PC through WEB TV: click into the picture – the computer mouse then moves the game cursor',
         'li: PS1 with the PlayStation mouse switched on: click into the picture')),
}


def kontrola(soubor):
    global chyb
    stary = subprocess.run(['git', 'show', '%s:app/src/main/assets/napoveda/%s' % (B317, soubor)], cwd=KOREN,
                           capture_output=True, text=True, check=True).stdout
    novy = open(os.path.join(KOREN, 'app/src/main/assets/napoveda', soubor), encoding='utf-8').read()
    a, b = rozbor(stary), rozbor(novy)
    tisk('%s: %d bloku textu v B317, %d v B318' % (soubor, len(a.bloky), len(b.bloky)))
    smi_pryc, smi_nove = POVOLENO[soubor]
    pryc, nove = [], []
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a.bloky, b.bloky, autojunk=False).get_opcodes():
        if op == 'equal':
            continue
        pryc += a.bloky[i1:i2]
        nove += b.bloky[j1:j2]
    for x in pryc:
        tisk('   B317  %s' % x[:240])
    for x in nove:
        tisk('   B318  %s' % x[:240])
    spatne_pryc = [x for x in pryc if not any(x.startswith(s) for s in smi_pryc)]
    spatne_nove = [x for x in nove if not any(x.startswith(s) for s in smi_nove)]
    ok = not spatne_pryc and not spatne_nove and len(pryc) == len(smi_pryc) and len(nove) == len(smi_nove)
    if not ok:
        chyb += 1
    tisk('%s  %s: zmenilo se jen to o mysi PS1 (%d bloky pryc, %d nove)%s' % (
        'OK    ' if ok else 'CHYBA ', soubor, len(pryc), len(nove),
        '' if ok else ' - NECEKANE: %s / %s' % ([x[:80] for x in spatne_pryc], [x[:80] for x in spatne_nove])))
    # znacky mimo zmenene bloky: kapitoly, odkazy, obrazky stejne (novy odkaz #tv v TIPu je povoleny)
    za, zb = [z for z in a.znacky], [z for z in b.znacky]
    rozdil = [z for z in difflib.ndiff([repr(x) for x in za], [repr(x) for x in zb]) if z[:1] in '+-']
    povolene = {"('li', None, None, None, None)", "('a', None, '#tv', None, None)", "('kbd', None, None, None, None)",
                "('span', None, None, None, 'nw')", "('span', None, None, None, 't')"}
    jine = [z for z in rozdil if z[2:] not in povolene]
    ok2 = not jine
    if not ok2:
        chyb += 1
    tisk('%s  %s: znacky HTML (kapitoly, odkazy, obrazky) stejne jako B317, navic jen polozky, odkaz na WEB TV a klavesy (%d zmen)%s' % (
        'OK    ' if ok2 else 'CHYBA ', soubor, len(rozdil), '' if ok2 else ' - JINE: %s' % jine[:4]))


def main():
    tisk('B318 - navod v appce: co se zmenilo proti B317 (%s)' % B317)
    kontrola('index.html')
    kontrola('en.html')
    tisk('VYSLEDEK: %d CHYBA' % chyb)
    with open(os.path.join(ZDE, 'VYSTUP_kontrola_navodu_b318.txt'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(vystup) + '\n')
    return 1 if chyb else 0


if __name__ == '__main__':
    sys.exit(main())
