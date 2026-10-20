#!/usr/bin/env python3
# B311: PODPISY JNI - Java NativeMenuBridge (javac -h proti android.jar) proti C++ nap_menu_gl.cpp.
# Kazda native metoda Javy musi mit v C++ funkci se stejnym jmenem, navratovym typem a typy
# parametru (jinak UnsatisfiedLinkError / spatna data za behu). Navic: funkce musi byt videt
# z knihovny (-fvisibility=hidden + JNIEXPORT) - to hlida readelf v JAK_SPUSTIT.
#   python3 kontrola_jni.py HLAVICKA_Z_JAVAC.h      -> VYSTUP_kontrola_jni.txt
import os, re, sys
ZDE = os.path.dirname(os.path.abspath(__file__))
h = open(sys.argv[1], encoding='utf-8').read()
c = open(os.path.join(ZDE, '../../app/src/main/cpp/menu/nap_menu_gl.cpp'), encoding='utf-8').read()
TYPY = {'JNIEnv', 'jclass', 'jobject', 'jint', 'jlong', 'jfloat', 'jdouble', 'jboolean', 'jbyte', 'jshort', 'jchar',
        'jstring', 'jbyteArray', 'jintArray', 'jfloatArray', 'jobjectArray', 'void'}
def protos(t):
    out = {}
    for m in re.finditer(r'JNIEXPORT\s+(\w+)\s+JNICALL\s+(Java_\w+)\s*\(([^)]*)\)', t):
        typy = []
        for p in m.group(3).split(','):
            sl = [x for x in re.sub(r'\bconst\b', '', p).replace('*', ' * ').split() if x]
            if not sl:
                continue
            typ = [x for x in sl if x in TYPY or x == '*']     # jmeno parametru zahodit
            typy.append(''.join(typ))
        out[m.group(2)] = (m.group(1), typy)
    return out
H, C = protos(h), protos(c)
vystup, chyb = [], 0
for k, (r, t) in sorted(H.items()):
    jm = k.replace('Java_eu_atarihelp_emu10_NativeMenuBridge_', '')
    if k not in C:
        chyb += 1; vystup.append('CHYBA  %-22s v C++ chybi' % jm); continue
    if (r, t) != C[k]:
        chyb += 1; vystup.append('CHYBA  %-22s Java %s(%s) / C++ %s(%s)' % (jm, r, ', '.join(t), C[k][0], ', '.join(C[k][1]))); continue
    vystup.append('OK     %-22s %s(%s)' % (jm, r, ', '.join(t[2:])))
for k in C:
    if k not in H:
        chyb += 1; vystup.append('CHYBA  %s je v C++, ale Java ji nema' % k)
vystup.append('VYSLEDEK: %d native metod v Jave, %d v C++, %d nesouhlasi' % (len(H), len(C), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_jni.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
