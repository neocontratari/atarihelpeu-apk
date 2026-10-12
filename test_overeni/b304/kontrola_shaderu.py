#!/usr/bin/env python3
# B304: vsechny shadery dema (nap_demo_shadery.h) prisne podle OpenGL ES 2.0
# (GLSL ES 1.00, "#version 100") pres referencni prekladac Khronos glslangValidator.
# Mesa na PC je shovivavejsi nez prekladace v telefonech - tohle odhali, co by
# na Mali / Adreno neproslo.
#   GLSLANG=/cesta/glslangValidator python3 kontrola_shaderu.py
import os, re, subprocess, sys, tempfile

KOREN = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'app', 'src', 'main', 'cpp', 'intro')
s = open(os.path.join(KOREN, 'nap_demo_shadery.h'), encoding='utf-8').read()
gl = open(os.path.join(KOREN, 'nap_demo_gl.h'), encoding='utf-8').read()
hlavicka = re.search(r'FS_HLAVICKA =\s*"(.*?)";', gl, re.S).group(1).encode().decode('unicode_escape')

defs = {}
for m in re.finditer(r'static const char \*(\w+)\s*=\s*(.*?);\n', s, re.S):
    jm, telo = m.group(1), m.group(2)
    txt = ''
    for a, b in re.findall(r'R"\((.*?)\)"|"(%SUM%)"', telo, re.S):
        txt += a if a else '%SUM%'
    defs[jm] = txt

glslang = os.environ.get('GLSLANG', 'glslangValidator')
chyb = 0
with tempfile.TemporaryDirectory() as d:
    for jm, txt in defs.items():
        if jm == 'SUM_GLSL':
            continue
        txt = txt.replace('%SUM%', defs['SUM_GLSL'])
        vs = jm.startswith('VS_')
        cesta = os.path.join(d, jm + ('.vert' if vs else '.frag'))
        with open(cesta, 'w') as f:
            f.write('#version 100\n' + ('' if vs else hlavicka) + txt)
        r = subprocess.run([glslang, cesta], capture_output=True, text=True)
        ok = r.returncode == 0
        print(('OK    ' if ok else 'CHYBA ') + jm)
        if not ok:
            chyb += 1
            print(r.stdout[-1500:])
print('VYSLEDEK: %d shaderu, %d chyb' % (len(defs) - 1, chyb))
sys.exit(1 if chyb else 0)
