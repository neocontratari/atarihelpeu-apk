#!/usr/bin/env python3
"""B320 (podle ../b319/kontrola_jni_ps1.py): PODPISY JNI vsech tri mostu, kterym B320 pridal herni ovladac -
NativePs1CoreBridge, NativeSegaCoreBridge a NativeAtariCoreBridge (hlavicky z javac -h proti android.jar)
proti C++ (nap_ps1_native.cpp, nap_sega_native_proof.cpp, atari/nap_atari_native.cpp). Kazda native metoda
Javy musi mit v C++ funkci se stejnym jmenem, navratovym typem a typy parametru (jinak UnsatisfiedLinkError
nebo spatna data za behu). Funkce, ktere jsou jen v C++ (starsi cesty, ktere Java uz nevola), se jen vypisi.
  python3 kontrola_jni.py ADRESAR_HLAVICEK_Z_JAVAC      -> VYSTUP_kontrola_jni.txt
"""
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
CPP = os.path.normpath(os.path.join(ZDE, '../../app/src/main/cpp'))
TYPY = {'JNIEnv', 'jclass', 'jobject', 'jint', 'jlong', 'jfloat', 'jdouble', 'jboolean', 'jbyte', 'jshort', 'jchar',
        'jstring', 'jbyteArray', 'jintArray', 'jshortArray', 'jfloatArray', 'jobjectArray', 'void'}
MOSTY = [
    # trida, C++, nove metody (B317 / B318 mys, B319 proud, B320 herni ovladac)
    ('NativePs1CoreBridge', 'nap_ps1_native.cpp',
     {'ps1MysDotyk': 'B317 mys', 'ps1MysNastav': 'B317 mys', 'ps1MysStav': 'B317 mys', 'ps1PcMys': 'B317 mys',
      'ps1MysZrychleni': 'B318 mys', 'proudOtevri2Native': 'B319 proud (volby stranky WEB TV)',
      'ps1OvladacKlavesa': 'B320 herni ovladac', 'ps1OvladacOsy': 'B320 herni ovladac', 'ps1OvladacPust': 'B320 herni ovladac'}),
    ('NativeSegaCoreBridge', 'nap_sega_native_proof.cpp',
     {'ovladacKlavesa': 'B320 herni ovladac', 'ovladacOsy': 'B320 herni ovladac', 'ovladacPust': 'B320 herni ovladac'}),
    ('NativeAtariCoreBridge', 'atari/nap_atari_native.cpp',
     {'devOvladacKlavesaNative': 'B320 herni ovladac', 'devOvladacOsyNative': 'B320 herni ovladac',
      'devOvladacPustNative': 'B320 herni ovladac'}),
]


def protos(t, predpona):
    out = {}
    for m in re.finditer(r'JNIEXPORT\s+(\w+)\s+JNICALL\s*\n?\s*(Java_\w+)\s*\(([^)]*)\)', t):
        if not m.group(2).startswith(predpona):
            continue
        typy = []
        for p in m.group(3).split(','):
            sl = [x for x in re.sub(r'\bconst\b', '', p).replace('*', ' * ').split() if x]
            if sl:
                typy.append(''.join(x for x in sl if x in TYPY or x == '*'))
        out[m.group(2)] = (m.group(1), typy)
    return out


vystup, chyb, metod = [], 0, 0
for trida, cpp, nove in MOSTY:
    predpona = 'Java_eu_atarihelp_emu10_%s_' % trida
    h = open(os.path.join(sys.argv[1], 'eu_atarihelp_emu10_%s.h' % trida), encoding='utf-8').read()
    c = open(os.path.join(CPP, cpp), encoding='utf-8').read()
    H, C = protos(h, predpona), protos(c, predpona)
    vystup.append('== %s (Java, javac -h) <-> %s (C++)' % (trida, cpp))
    for k, (r, t) in sorted(H.items()):
        jm = k[len(predpona):]
        metod += 1
        if k not in C:
            chyb += 1
            vystup.append('CHYBA  %-26s v C++ chybi' % jm)
        elif (r, t) != C[k]:
            chyb += 1
            vystup.append('CHYBA  %-26s Java %s(%s) / C++ %s(%s)' % (jm, r, ', '.join(t), C[k][0], ', '.join(C[k][1])))
        else:
            vystup.append('OK     %-26s %s(%s)%s' % (jm, r, ', '.join(t[2:]), ('   <- ' + nove[jm]) if jm in nove else ''))
    for k in sorted(C):
        if k not in H:
            vystup.append('       %-26s jen v C++ (Java ji nevola)' % k[len(predpona):])
    chybi_nove = [n for n in nove if predpona + n not in H]
    if chybi_nove:
        chyb += 1
        vystup.append('CHYBA  v Jave chybi nove metody: %s' % ', '.join(sorted(chybi_nove)))
    vystup.append('')
vystup.append('VYSLEDEK: %d native metod Javy ve 3 mostech, %d CHYB' % (metod, chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_jni.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
print('\n'.join(vystup))
sys.exit(1 if chyb else 0)
