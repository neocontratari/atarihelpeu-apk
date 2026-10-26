#!/usr/bin/env python3
# B314 (podle ../b313/kontrola_jni.py): PODPISY JNI - Java (javac -h proti android.jar) proti C++.
#   NativeMenuBridge  <-> cpp/menu/nap_menu_gl.cpp    (B314: 24 native metod - nova menuJazyk)
#   NativeIntroBridge <-> cpp/intro/nap_intro_gl.cpp  (B314: nova introJazyk)
# Kazda native metoda Javy musi mit v C++ funkci se stejnym jmenem, navratovym typem a typy
# parametru (jinak UnsatisfiedLinkError / spatna data za behu) a naopak.
#   python3 kontrola_jni.py ADRESAR_HLAVICEK_Z_JAVAC      -> VYSTUP_kontrola_jni.txt
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
CPP = os.path.normpath(os.path.join(ZDE, '../../app/src/main/cpp'))
TYPY = {'JNIEnv', 'jclass', 'jobject', 'jint', 'jlong', 'jfloat', 'jdouble', 'jboolean', 'jbyte', 'jshort', 'jchar',
        'jstring', 'jbyteArray', 'jintArray', 'jfloatArray', 'jobjectArray', 'void'}
MOSTY = [('NativeMenuBridge', 'menu/nap_menu_gl.cpp', 24, 'menuJazyk'),
         ('NativeIntroBridge', 'intro/nap_intro_gl.cpp', None, 'introJazyk')]


def protos(t, predpona):
    out = {}
    for m in re.finditer(r'JNIEXPORT\s+(\w+)\s+JNICALL\s+(Java_\w+)\s*\(([^)]*)\)', t):
        if not m.group(2).startswith(predpona):
            continue
        typy = []
        for p in m.group(3).split(','):
            sl = [x for x in re.sub(r'\bconst\b', '', p).replace('*', ' * ').split() if x]
            if not sl:
                continue
            typy.append(''.join(x for x in sl if x in TYPY or x == '*'))
        out[m.group(2)] = (m.group(1), typy)
    return out


adr = sys.argv[1]
vystup, chyb = [], 0
for most, zdroj, pocet, nova in MOSTY:
    predpona = 'Java_eu_atarihelp_emu10_' + most + '_'
    h = open(os.path.join(adr, 'eu_atarihelp_emu10_' + most + '.h'), encoding='utf-8').read()
    c = open(os.path.join(CPP, zdroj), encoding='utf-8').read()
    H, C = protos(h, predpona), protos(c, predpona)
    vystup.append('== %s (Java) <-> %s (C++)' % (most, zdroj))
    for k, (r, t) in sorted(H.items()):
        jm = k[len(predpona):]
        if k not in C:
            chyb += 1
            vystup.append('CHYBA  %-24s v C++ chybi' % jm)
            continue
        if (r, t) != C[k]:
            chyb += 1
            vystup.append('CHYBA  %-24s Java %s(%s) / C++ %s(%s)' % (jm, r, ', '.join(t), C[k][0], ', '.join(C[k][1])))
            continue
        vystup.append('OK     %-24s %s(%s)%s' % (jm, r, ', '.join(t[2:]), '   <- B314' if jm == nova else ''))
    for k in C:
        if k not in H:
            chyb += 1
            vystup.append('CHYBA  %s je v C++, ale Java ji nema' % k)
    if pocet is not None and len(H) != pocet:
        chyb += 1
        vystup.append('CHYBA  cekano %d native metod, Java ma %d' % (pocet, len(H)))
    if (predpona + nova) not in H:
        chyb += 1
        vystup.append('CHYBA  chybi nova metoda %s' % nova)
    vystup.append('       %s: %d native metod v Jave, %d v C++' % (most, len(H), len(C)))
vystup.append('VYSLEDEK: %d nesouhlasi' % chyb)
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_jni.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
