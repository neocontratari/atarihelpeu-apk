#!/usr/bin/env python3
"""B317 (podle ../b314/kontrola_jni.py): PODPISY JNI - NativePs1CoreBridge (javac -h proti android.jar)
proti nap_ps1_native.cpp. Kazda native metoda Javy musi mit v C++ funkci se stejnym jmenem, navratovym
typem a typy parametru (jinak UnsatisfiedLinkError nebo spatna data za behu). Funkce, ktere jsou jen
v C++ (starsi cesty ps1Egl*, ktere Java uz nevola), se jen vypisi.
  python3 kontrola_jni_ps1.py ADRESAR_HLAVICEK_Z_JAVAC      -> VYSTUP_kontrola_jni_ps1.txt
"""
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
CPP = os.path.normpath(os.path.join(ZDE, '../../app/src/main/cpp/nap_ps1_native.cpp'))
TYPY = {'JNIEnv', 'jclass', 'jobject', 'jint', 'jlong', 'jfloat', 'jdouble', 'jboolean', 'jbyte', 'jshort', 'jchar',
        'jstring', 'jbyteArray', 'jintArray', 'jshortArray', 'jfloatArray', 'jobjectArray', 'void'}
NOVE = {'ps1MysDotyk', 'ps1MysNastav', 'ps1MysStav', 'ps1PcMys'}
PREDPONA = 'Java_eu_atarihelp_emu10_NativePs1CoreBridge_'


def protos(t):
    out = {}
    for m in re.finditer(r'JNIEXPORT\s+(\w+)\s+JNICALL\s*\n?\s*(Java_\w+)\s*\(([^)]*)\)', t):
        if not m.group(2).startswith(PREDPONA):
            continue
        typy = []
        for p in m.group(3).split(','):
            sl = [x for x in re.sub(r'\bconst\b', '', p).replace('*', ' * ').split() if x]
            if sl:
                typy.append(''.join(x for x in sl if x in TYPY or x == '*'))
        out[m.group(2)] = (m.group(1), typy)
    return out


h = open(os.path.join(sys.argv[1], 'eu_atarihelp_emu10_NativePs1CoreBridge.h'), encoding='utf-8').read()
c = open(CPP, encoding='utf-8').read()
H, C = protos(h), protos(c)
vystup, chyb = ['== NativePs1CoreBridge (Java, javac -h) <-> nap_ps1_native.cpp (C++)'], 0
for k, (r, t) in sorted(H.items()):
    jm = k[len(PREDPONA):]
    if k not in C:
        chyb += 1
        vystup.append('CHYBA  %-22s v C++ chybi' % jm)
    elif (r, t) != C[k]:
        chyb += 1
        vystup.append('CHYBA  %-22s Java %s(%s) / C++ %s(%s)' % (jm, r, ', '.join(t), C[k][0], ', '.join(C[k][1])))
    else:
        vystup.append('OK     %-22s %s(%s)%s' % (jm, r, ', '.join(t[2:]), '   <- B317 mys' if jm in NOVE else ''))
for k in sorted(C):
    if k not in H:
        vystup.append('       %-22s jen v C++ (Java ji nevola)' % k[len(PREDPONA):])
chybi_nove = [n for n in NOVE if PREDPONA + n not in H]
if chybi_nove:
    chyb += 1
    vystup.append('CHYBA  v Jave chybi nove metody mysi: %s' % ', '.join(chybi_nove))
vystup.append('VYSLEDEK: %d native metod Javy, %d CHYB' % (len(H), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_jni_ps1.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
print('\n'.join(vystup))
sys.exit(1 if chyb else 0)
