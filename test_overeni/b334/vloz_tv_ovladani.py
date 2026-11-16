#!/usr/bin/env python3
"""B334: vlozi spolecne/nap_tv_ovladani.js (dalkovy ovladac TV / gamepad na strankach appky) jako PRVNI
skript do stranek appky - hned za <meta charset> (skript si musi poznamenat posluchace klepnuti drive,
nez je stranka prida). Opakovane spusteni nic nezmeni. Bez parametru: vlozit; --kontrola: jen overit.
Pouziti: python3 vloz_tv_ovladani.py [--kontrola]
"""
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets'))
STRANKY = ['index.html', 'hry/index.html', 'hry/aktivace.html', 'hry/odemknout.html', 'hry/jazyk.html',
           'napoveda/index.html', 'napoveda/en.html', 'player/index.html', 'dj/index.html',
           'emu_ps1/index.html', 'emu_sega/index.html', 'emu_atari_cpp/index.html',
           'intro/index.html', 'intro/etapa1.html', 'intro/etapa2.html', 'intro/etapa5.html']
kontrola = '--kontrola' in sys.argv
chyb = 0
for s in STRANKY:
    p = os.path.join(A, s)
    t = open(p, encoding='utf-8').read()
    cesta = ('../' if '/' in s else '') + 'spolecne/nap_tv_ovladani.js'
    znacka = '<script src="%s"></script>' % cesta
    m = re.search(r'<meta charset="utf-8">\n?', t, re.I)
    if znacka in t:
        prvni = t.index('<script')
        ok = t.index(znacka) == prvni
        print('%s  %s (prvni skript stranky: %s)' % ('OK    ' if ok else 'CHYBA ', s, 'ano' if ok else 'NE'))
        chyb += 0 if ok else 1
        continue
    if kontrola or not m:
        print('CHYBA  %s - skript chybi' % s)
        chyb += 1
        continue
    vlozit = '<!-- B334: dalkovy ovladac TV / gamepad (sipky + OK) - musi byt prvni skript -->\n' + znacka + '\n'
    t = t[:m.end()] + ('' if t[m.end() - 1] == '\n' else '\n') + vlozit + t[m.end():]
    open(p, 'w', encoding='utf-8').write(t)
    print('VLOZENO %s' % s)
print('VYSLEDEK: %d stranek, %d CHYBA' % (len(STRANKY), chyb))
sys.exit(1 if chyb else 0)
