#!/bin/bash
# B307: vsechny testy uvodniho dema najednou (vystupy VYSTUP_* sem do test_overeni/b307)
# Spoustet odkudkoli: bash test_overeni/b307/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt):
#   PRAC     pracovni adresar (prelozene testy, snimky, logy)
#   INC      hlavicky EGL / GLES2 / KHR (jako ../b304),  JAVA_HOME = JDK
#   GLSLANG  Khronos glslangValidator,  Chromium + Playwright pro Python, knihovny Mesa, zlib
#   HRA      (nepovinne) Reneho Pitt-Kitt Platinum .xex - overi, ze klip v assets vznikl z hry
set -u
PRAC=${PRAC:-/tmp/nap_b307}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
GLSLANG=${GLSLANG:-glslangValidator}
HRA=${HRA:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b307
export EGL_PLATFORM=surfaceless
INTRO="-I../b306/stub -I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/intro"

echo "== 1) cele demo po 0,25 s na vysku (540x1170) a na sirku / TV (854x480): shadery, chyby GL, jas"
g++ -std=c++17 -O2 -Wno-unused-function -I$INC -o $PRAC/demo_snimky demo_snimky.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz || echo CHYBA_PREKLAD
rm -rf $PRAC/vse_v $PRAC/vse_s; mkdir -p $PRAC/vse_v $PRAC/vse_s
$PRAC/demo_snimky $PRAC/vse_v 540 1170 --vse 0.25 > $PRAC/vse_v.txt 2>&1
$PRAC/demo_snimky $PRAC/vse_s 854 480 --vse 0.25 > $PRAC/vse_s.txt 2>&1
{
  echo "B307 - cele demo (45 s) po 0,25 s, stejny C++ jako v telefonu (Mesa, OpenGL ES 2.0 bez okna)"
  for f in vse_v vse_s; do
    echo "=== $( [ $f = vse_v ] && echo 'telefon na vysku 540x1170' || echo 'na sirku / TV 854x480' ) ==="
    grep -v "^t=\|^DEMO" $PRAC/$f.txt | sed "s#$PRAC/##"
    echo "snimku: $(grep -c '^t=' $PRAC/$f.txt), chyb GL: $(grep -c 'GL chyba' $PRAC/$f.txt)"
    echo "scena 4 (27-37 s: pruty, konzole, trojice, CRT, defile):"
    grep "^t=" $PRAC/$f.txt | awk '$2>=26.99 && $2<=37.01' | sed "s#$PRAC/##"
  done
} > VYSTUP_demo_snimky.txt
grep "^snimku" VYSTUP_demo_snimky.txt

echo "== 2) zvuk dema (stopa + zvuk hry Pitt-Kitt)"
g++ -std=c++17 -O2 -Wno-unused-function -o $PRAC/demo_zvuk demo_zvuk.cpp && $PRAC/demo_zvuk $PRAC/demo.wav > VYSTUP_demo_zvuk.txt 2>&1; tail -2 VYSTUP_demo_zvuk.txt

echo "== 3) tlacitko SKIP: nakreslene = tam, kde ho hleda dotyk"
g++ -std=c++17 -O2 -Wno-unused-function -I$INC -o $PRAC/test_skip test_skip.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz && $PRAC/test_skip 2>&1 | grep -v "^DEMO" > VYSTUP_test_skip.txt; tail -1 VYSTUP_test_skip.txt

echo "== 4) SKUTECNY nap_intro_gl.cpp na PC (OpenSL, dotyk, cela stopa, OpenSL nejde, chybi klip) + TSan"
mkdir -p $PRAC/assety_bez_klipu/design
ln -sf $R/app/src/main/assets/design/nap_logo_original.png $PRAC/assety_bez_klipu/design/nap_logo_original.png
g++ -std=c++17 -O2 -pthread -Wno-unused-function $INTRO -o $PRAC/test_intro_host test_intro_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 && \
  $PRAC/test_intro_host $PRAC/assety_bez_klipu 2>&1 | grep -v "^DEMO\|^NAP_DEMO" > VYSTUP_test_intro_host.txt; tail -1 VYSTUP_test_intro_host.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $INTRO -o $PRAC/test_intro_host_tsan test_intro_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 && \
  NAP_RYCHLOST=2 TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 detect_deadlocks=0" timeout 400 $PRAC/test_intro_host_tsan $PRAC/assety_bez_klipu > $PRAC/tsan_intro.txt 2>&1
# (detect_deadlocks=0: zamky Mesy - vlakna llvmpipe zamykaji v jednom vlakne a odemykaji v jinem - jinak shodi
#  vlastni hlidac zamku TSan "CHECK failed: sanitizer_deadlock_detector1.cpp"; zavody v pameti TSan hlida dal)
PRAC=$PRAC python3 - <<'EOF' > VYSTUP_tsan.txt
import os, re
t = open(os.environ['PRAC'] + '/tsan_intro.txt', errors='replace').read()
nase = 0; m = 0
for b in t.split('=================='):
    if 'WARNING: ThreadSanitizer' not in b: continue
    sek = [s for s in re.split(r'\n\s*\n', b) if re.search(r'(Write|Read|atomic read|atomic write) of size', s)]
    def prvni(s):
        for l in s.split('\n'):
            if not re.match(r'\s*#\d+ ', l) or 'libtsan' in l or 'tsan_interceptors' in l: continue
            return 'libgallium' not in l and 'libEGL' not in l and '<null> <null>' not in l
        return False
    if any(prvni(s) for s in sek): nase += 1
    else: m += 1
vys = [l for l in t.split('\n') if l.startswith('OK') or l.startswith('CHYBA') or l.startswith('VYSLEDEK')]
print('B307 - test_intro_host pod ThreadSanitizer (GL vlakno, OpenSL zpetne volani, vlakno zvuku, rozbocovac, dotyk z JNI)')
print('karta 2x rychleji (NAP_RYCHLOST=2): pod TSan se stopa pocita ~10x pomaleji; detect_deadlocks=0 (zamky Mesy)')
if 'CHECK failed' in t: print('POZOR: TSan sam spadl (CHECK failed) - vysledek neuplny')
for l in vys: print('  ' + l[:160])
print('hlaseni TSan: v nasem kodu %d, uvnitr Mesy (libgallium / libEGL) %d' % (nase, m))
EOF
tail -1 VYSTUP_tsan.txt

echo "== 5) JVM: skutecne tridy NativeIntroBridge a NapDemoView se skutecnym C++"
mkdir -p $PRAC/jvmlib $PRAC/jvmout $PRAC/jvmout2
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $INTRO -o $PRAC/jvmlib/libnapintro.so tv_intro.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2
javac -d $PRAC/jvmout -sourcepath ../b306/jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NativeIntroBridge.java jvm/TestPodpisu.java 2>&1 | grep -v JAVA_TOOL
NAP_JVM_INTRO=1 timeout 120 java -Djava.library.path=$PRAC/jvmlib -cp $PRAC/jvmout TestPodpisu 2>&1 | grep -v "JAVA_TOOL\|^DEMO\|^NAP" > jvm/VYSTUP_TestPodpisu.txt; tail -1 jvm/VYSTUP_TestPodpisu.txt
javac -d $PRAC/jvmout2 -sourcepath jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NativeIntroBridge.java ../../app/src/main/java/eu/atarihelp/emu10/NapDemoView.java jvm/eu/atarihelp/emu10/TestNapDemoView.java 2>&1 | grep -v JAVA_TOOL
NAP_JVM_INTRO=1 timeout 120 java -Djava.library.path=$PRAC/jvmlib -cp $PRAC/jvmout2 eu.atarihelp.emu10.TestNapDemoView 2>&1 | grep -v "JAVA_TOOL\|^DEMO\|^NAP" > jvm/VYSTUP_TestNapDemoView.txt; tail -1 jvm/VYSTUP_TestNapDemoView.txt

echo "== 6) shadery prisne podle GLSL ES 1.00 (glslangValidator)"
GLSLANG=$GLSLANG python3 kontrola_shaderu.py > /dev/null; tail -1 VYSTUP_kontrola_shaderu.txt

echo "== 7) prohlizec (Chromium): demo na TV / PC proudem z C++ vcetne Pitt-Kitt"
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $INTRO -o $PRAC/libintro_b307.so tv_intro.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2
PRAC=$PRAC timeout 300 python3 test_b307_prohlizec.py > $PRAC/log_prohlizec.txt 2>&1; tail -1 VYSTUP_test_b307_prohlizec.txt

echo "== 8) 16bit (procesor) a 32bit (GPU) ukazka samostatne: rychlost"
g++ -std=c++17 -O2 -Wno-unused-function -o $PRAC/ukazka16 ukazka16.cpp && mkdir -p $PRAC/u16 && $PRAC/ukazka16 $PRAC/u16 > $PRAC/u16.txt 2>&1
g++ -std=c++17 -O2 -Wno-unused-function -I$INC -o $PRAC/ukazka32 ukazka32.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 && mkdir -p $PRAC/u32 && $PRAC/ukazka32 $PRAC/u32 0 0.3 0.6 0.9 1.2 > $PRAC/u32.txt 2>&1
{ echo "B307 - ukazky na CRT samostatne (PC, jedno jadro)"; echo "16bit (procesor, 320x224):"; sed 's/^/  /' $PRAC/u16.txt; echo "32bit (GPU Mesa llvmpipe, 320x240):"; sed 's/^/  /' $PRAC/u32.txt; } > VYSTUP_ukazky.txt
tail -3 VYSTUP_ukazky.txt

if [ -n "$HRA" ] && [ -f "$HRA" ]; then
  echo "== 9) klip Pitt-Kitt znovu z Reneho hry (SKUTECNE jadro Atari 130XE z appky)"
  g++ -std=c++17 -O2 -Wno-unused-function -Wno-unused-result -I../../app/src/main/cpp/atari -o $PRAC/nahraj_hru nahraj_hru.cpp -lz
  $PRAC/nahraj_hru "$HRA" zaznam $PRAC/klip_znovu.bin "110-129,232-251,440-519" "200-206:START,420-426:START" > $PRAC/nahraj.txt 2>&1
  { echo "B307 - klip intro/pittkitt.klip znovu nahrany z Reneho Pitt-Kitt Platinum (.xex):"; sed "s#$PRAC/##" $PRAC/nahraj.txt;
    if cmp -s $PRAC/klip_znovu.bin ../../app/src/main/assets/intro/pittkitt.klip; then echo "VYSLEDEK: klip v assets je bajt po bajtu stejny (sha256 $(sha256sum $PRAC/klip_znovu.bin | cut -c1-16)...)"; else echo "VYSLEDEK: CHYBA - klip se lisi"; fi; } > VYSTUP_klip.txt
  tail -1 VYSTUP_klip.txt
fi
