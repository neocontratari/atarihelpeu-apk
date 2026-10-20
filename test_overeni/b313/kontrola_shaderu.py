#!/usr/bin/env python3
# B313 (kopie ../b311): VSECHNY shadery hlavniho menu (nap_menu_shadery.h, nap_menu_umeni_dzubox.h,
# nap_menu_umeni_web.h) prisne podle OpenGL ES 2.0 (GLSL ES 1.00, "#version 100") pres
# referencni prekladac Khronos glslangValidator - presne v tom tvaru, v jakem je appka preda
# GPU (fragment shader = FS_HLAVICKA + text, %SUM% = SUM_GLSL, jako Prog::sestav a sSumem).
# Navic se kazda dvojice z kodu (sestav(VS, FS)) zkusi spojit (-l): sedi varying mezi nimi?
# Mesa na PC je shovivavejsi nez prekladace v telefonech - tohle odhali, co by na Mali /
# Adreno / PowerVR neproslo (napr. int % nebo pow zaporneho cisla).
#   GLSLANG=/cesta/glslangValidator python3 kontrola_shaderu.py      -> VYSTUP_kontrola_shaderu.txt
import os
import re
import subprocess
import sys
import tempfile

ZDE = os.path.dirname(os.path.abspath(__file__))
CPP = os.path.normpath(os.path.join(ZDE, '../../app/src/main/cpp'))
gl = open(os.path.join(CPP, 'intro/nap_demo_gl.h'), encoding='utf-8').read()
hlavicka = re.search(r'FS_HLAVICKA =\s*"(.*?)";', gl, re.S).group(1).encode().decode('unicode_escape')


def nacti(soubor, defs, odkud, prefix=''):
    s = open(os.path.join(CPP, soubor), encoding='utf-8').read()
    for m in re.finditer(r'static const char \*(\w+)\s*=\s*((?:\s*(?:R"\(.*?\)"|"(?:[^"\\]|\\.)*"))+)\s*;', s, re.S):
        jm, telo = m.group(1), m.group(2)
        txt = ''
        for a, b in re.findall(r'R"\((.*?)\)"|"((?:[^"\\]|\\.)*)"', telo, re.S):
            txt += a if a else ('%SUM%' if b == '%SUM%' else b.encode().decode('unicode_escape'))
        if txt:
            defs[prefix + jm] = txt
            odkud[prefix + jm] = soubor


menu, odkud = {}, {}
for f in ('menu/nap_menu_shadery.h', 'menu/nap_menu_umeni_dzubox.h', 'menu/nap_menu_umeni_web.h'):
    nacti(f, menu, odkud)
demo, odkudDemo = {}, {}
for f in ('intro/nap_demo_shadery.h', 'intro/nap_demo_konzole.h'):
    nacti(f, demo, odkudDemo, 'napdemo::')
SUM = demo['napdemo::SUM_GLSL']
vse = dict(menu)
vse.update(demo)
odkud.update(odkudDemo)

glslang = os.environ.get('GLSLANG', 'glslangValidator')
vystup = []
chyb = 0


def tisk(x):
    print(x, flush=True)
    vystup.append(x)


def soubor(d, jm):
    txt = vse[jm].replace('%SUM%', SUM)
    vs = jm.split('::')[-1].startswith('VS_')
    cesta = os.path.join(d, jm.replace('::', '_') + ('.vert' if vs else '.frag'))
    with open(cesta, 'w') as f:
        f.write('#version 100\n' + ('' if vs else hlavicka) + txt)
    return cesta


# dvojice presne z kodu menu: X.sestav(VS, FS) / sSumem(FS)
dvojice = []
for f in ('menu/nap_menu_kresba.h', 'menu/nap_menu_umeni.h', 'menu/nap_menu_umeni_3d.h', 'menu/nap_menu_umeni_dzubox.h',
          'menu/nap_menu_umeni_help.h', 'menu/nap_menu_umeni_kazeta.h', 'menu/nap_menu_umeni_web.h'):
    s = open(os.path.join(CPP, f), encoding='utf-8').read()
    for m in re.finditer(r'\.sestav\(([\w:]+),\s*(?:sSumem\()?([\w:]+)\)?(?:\.c_str\(\))?,([^;]*)\);', s):
        jmena = re.findall(r'"([^"]+)"', m.group(3))
        dvojice.append((m.group(1), m.group(2), '/'.join(jmena) or '?', f))

tisk('B313 - shadery hlavniho menu a dema (vcetne noveho FS_KONZOLE) podle GLSL ES 1.00 (OpenGL ES 2.0), Khronos glslangValidator')
with tempfile.TemporaryDirectory() as d:
    for jm in sorted(menu, key=lambda x: (odkud[x], x)):
        if jm in ('SUM_GLSL',):
            continue
        r = subprocess.run([glslang, soubor(d, jm)], capture_output=True, text=True)
        ok = r.returncode == 0
        tisk(('OK    ' if ok else 'CHYBA ') + '%-18s (%s)' % (jm, odkud[jm]))
        if not ok:
            chyb += 1
            tisk(r.stdout[-1500:])
    tisk('--- programy menu (vertex + fragment spojene, -l) ---')
    for vs, fs, popis, f in dvojice:
        vs = vs if vs in vse else 'napdemo::' + vs.split('::')[-1]
        fs = fs if fs in vse else 'napdemo::' + fs.split('::')[-1]
        if vs not in vse or fs not in vse:
            chyb += 1
            tisk('CHYBA %-20s %s + %s - shader nenalezen' % (popis, vs, fs))
            continue
        r = subprocess.run([glslang, '-l', soubor(d, vs), soubor(d, fs)], capture_output=True, text=True)
        ok = r.returncode == 0
        tisk(('OK    ' if ok else 'CHYBA ') + '%-20s %s + %s' % (popis, vs, fs))
        if not ok:
            chyb += 1
            tisk(r.stdout[-1500:])
n = len([j for j in menu if j != 'SUM_GLSL'])
tisk('VYSLEDEK: %d shaderu menu, %d programu, %d chyb' % (n, len(dvojice), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_shaderu.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
