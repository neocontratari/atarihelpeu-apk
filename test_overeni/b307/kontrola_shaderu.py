#!/usr/bin/env python3
# B307: VSECHNY shadery dema (nap_demo_shadery.h + B307 nap_demo_konzole.h, nap_demo_32bit.h)
# prisne podle OpenGL ES 2.0 (GLSL ES 1.00, "#version 100") pres referencni prekladac
# Khronos glslangValidator - presne v tom tvaru, v jakem je appka preda GPU
# (fragment shader = FS_HLAVICKA + text, %SUM% = SUM_GLSL). Mesa na PC je shovivavejsi
# nez prekladace v telefonech - tohle odhali, co by na Mali / Adreno / PowerVR neproslo.
#   GLSLANG=/cesta/glslangValidator python3 kontrola_shaderu.py      -> VYSTUP_kontrola_shaderu.txt
import os, re, subprocess, sys, tempfile

KOREN = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'app', 'src', 'main', 'cpp', 'intro')
gl = open(os.path.join(KOREN, 'nap_demo_gl.h'), encoding='utf-8').read()
hlavicka = re.search(r'FS_HLAVICKA =\s*"(.*?)";', gl, re.S).group(1).encode().decode('unicode_escape')

defs = {}
odkud = {}
for soubor in ('nap_demo_shadery.h', 'nap_demo_konzole.h', 'nap_demo_32bit.h'):
    s = open(os.path.join(KOREN, soubor), encoding='utf-8').read()
    # static const char *JMENO = R"(...)" ["%SUM%" R"(...)"] ;   (retezce az ke strednku za nimi)
    for m in re.finditer(r'static const char \*(\w+)\s*=\s*((?:\s*(?:R"\(.*?\)"|"(?:[^"\\]|\\.)*"))+)\s*;', s, re.S):
        jm, telo = m.group(1), m.group(2)
        txt = ''
        for a, b in re.findall(r'R"\((.*?)\)"|"((?:[^"\\]|\\.)*)"', telo, re.S):
            txt += a if a else ('%SUM%' if b == '%SUM%' else b.encode().decode('unicode_escape'))
        if txt:
            defs[jm] = txt
            odkud[jm] = soubor
    # B307: vertex shader 32bitove ukazky vraceny funkci (VS_PLNY32)
    for m in re.finditer(r'static const char \*(VS_\w+)\(\)\s*\{\s*return\s*((?:"(?:[^"\\]|\\.)*"\s*)+);', s, re.S):
        txt = ''.join(x.encode().decode('unicode_escape') for x in re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2)))
        defs[m.group(1)] = txt
        odkud[m.group(1)] = soubor

glslang = os.environ.get('GLSLANG', 'glslangValidator')
chyb = 0
vystup = []
def tisk(x):
    print(x)
    vystup.append(x)
tisk('B307 - kontrola shaderu dema podle GLSL ES 1.00 (OpenGL ES 2.0), Khronos glslangValidator')
with tempfile.TemporaryDirectory() as d:
    for jm, txt in sorted(defs.items(), key=lambda x: (odkud[x[0]], x[0])):
        if jm == 'SUM_GLSL' or jm == 'FS_HLAVICKA':
            continue
        txt = txt.replace('%SUM%', defs['SUM_GLSL'])
        vs = jm.startswith('VS_')
        cesta = os.path.join(d, jm + ('.vert' if vs else '.frag'))
        with open(cesta, 'w') as f:
            f.write('#version 100\n' + ('' if vs else hlavicka) + txt)
        r = subprocess.run([glslang, cesta], capture_output=True, text=True)
        ok = r.returncode == 0
        tisk(('OK    ' if ok else 'CHYBA ') + '%-14s (%s)' % (jm, odkud[jm]))
        if not ok:
            chyb += 1
            tisk(r.stdout[-1500:])
n = len([j for j in defs if j not in ('SUM_GLSL', 'FS_HLAVICKA')])
tisk('VYSLEDEK: %d shaderu, %d chyb' % (n, chyb))
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'VYSTUP_kontrola_shaderu.txt'), 'w').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
