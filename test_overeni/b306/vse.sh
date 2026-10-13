#!/bin/bash
# B306: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b306)
# Spoustet odkudkoli: bash test_overeni/b306/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt a ../b305/JAK_SPUSTIT.txt):
#   PRAC    pracovni adresar (prelozene testy, logy)
#   PNG320  snimky 3D sceny 320x240, PNG640 snimky 640x480 (../b305/JAK_SPUSTIT.txt krok 0)
#   CLOWN   objekty jadra clownmdemu *.o (../b305/JAK_SPUSTIT.txt 3a)
#   INC     hlavicky EGL / GLES2 / KHR,  JAVA_HOME = JDK
#   Chromium + Playwright pro Python, knihovny Mesa (libEGL, libGLESv2)
set -u
PRAC=${PRAC:-/tmp/nap_b306}
PNG320=${PNG320:-$PRAC/ps1_320}
PNG640=${PNG640:-$PRAC/ps1_640}
CLOWN=${CLOWN:-$PRAC/clown}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b306
V=../../app/src/main/cpp/vendor/clownmdemu-core
SEGA="-DNAP_SEGA_VENDOR_CORE_PRESENT=1 -DCC_USE_C99_INTEGERS=1 -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clownz80/source"
PS1="-DANDROID -I../b305/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/vendor/stb"
INTRO="-Istub -I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/intro"
T="-I../../app/src/main/cpp/tvproud"
echo "== knihovny"
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $SEGA -o libsega_b306.so tv_sega6.cpp $CLOWN/*.o -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_SEGA
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -o libps1_b306.so tv_ps1_6.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PS1
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $INTRO -o libintro_b306.so tv_intro.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_INTRO
# B305 (git HEAD) pro srovnani: stare nap_sega_native_proof.cpp / nap_ps1_native.cpp ve stejne strukture adresaru
# (34c5fa3 = commit B305)
REF=${REF:-34c5fa3}
M=$PRAC/b305src
rm -rf $M && mkdir -p $M/app/src/main/cpp $M/test_overeni/b306 $M/test_overeni/b305
(cd $R && git show $REF:app/src/main/cpp/nap_sega_native_proof.cpp > $M/app/src/main/cpp/nap_sega_native_proof.cpp && git show $REF:app/src/main/cpp/nap_ps1_native.cpp > $M/app/src/main/cpp/nap_ps1_native.cpp)
ln -s $R/app/src/main/cpp/tvproud $M/app/src/main/cpp/tvproud; ln -s $R/app/src/main/cpp/vendor $M/app/src/main/cpp/vendor
cp tv_sega6.cpp tv_ps1_6.cpp $M/test_overeni/b306/; ln -s $R/test_overeni/b305/sega_hra.h $M/test_overeni/b305/sega_hra.h
VA=$R/app/src/main/cpp/vendor/clownmdemu-core
SEGA_ABS="-DNAP_SEGA_VENDOR_CORE_PRESENT=1 -DCC_USE_C99_INTEGERS=1 -I$R/test_overeni/b291/stub -I$INC -I$JV/include -I$JV/include/linux -I$VA/source -I$VA/libraries/clowncommon -I$VA/libraries/clown68000/source/interpreter -I$VA/libraries/clownz80/source"
(cd $M/test_overeni/b306 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function -DNAP_STARY_B305 $SEGA_ABS -o $R/test_overeni/b306/libsega_b305srovnani.so tv_sega6.cpp $CLOWN/*.o -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_SEGA305)
(cd $M/test_overeni/b306 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function -DANDROID -I$R/test_overeni/b305/stub -I$R/test_overeni/b291/stub -I$INC -I$JV/include -I$JV/include/linux -I$R/app/src/main/cpp/vendor/stb -o $R/test_overeni/b306/libps1_b305srovnani.so tv_ps1_6.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PS1_305)

echo "== 1) prohlizec (Chromium bez autoplay)"; timeout 900 python3 test_b306_prohlizec.py $PNG320 > $PRAC/log_prohlizec.txt 2>&1; tail -1 VYSTUP_test_b306_prohlizec.txt
echo "== 2) srovnani B305 / B306"; timeout 900 python3 test_b306_prohlizec.py $PNG320 --srovnani > $PRAC/log_srovnani.txt 2>&1; tail -1 VYSTUP_srovnani_b305_b306.txt
echo "== 3) Sega 60/s (skutecne vlakno emulace + jadro)"
g++ -std=c++17 -O2 -pthread -Wno-unused-function $SEGA -o $PRAC/test_sega_60 test_sega_60.cpp $CLOWN/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 && $PRAC/test_sega_60 2>&1 | grep -v "^NAP_SEGA\|BUILD2" > VYSTUP_test_sega_60.txt; tail -1 VYSTUP_test_sega_60.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $SEGA -o $PRAC/test_sega_60_tsan test_sega_60.cpp $CLOWN/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 && TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0" $PRAC/test_sega_60_tsan > $PRAC/tsan_sega60.txt 2>&1
echo "== 4) demo (OpenSL v C++)"
g++ -std=c++17 -O2 -pthread -Wno-unused-function $INTRO -o $PRAC/test_intro_host test_intro_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 && $PRAC/test_intro_host 2>&1 | grep -v "^DEMO\|^NAP_DEMO" > VYSTUP_test_intro_host.txt; tail -1 VYSTUP_test_intro_host.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $INTRO -o $PRAC/test_intro_host_tsan test_intro_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 && TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0" $PRAC/test_intro_host_tsan > $PRAC/tsan_intro.txt 2>&1
echo "== 5) rozbocovac: pakety jen se zvukem"
g++ -std=c++17 -O2 -pthread $T -o $PRAC/test_hub6 test_hub6.cpp && $PRAC/test_hub6 > VYSTUP_test_hub6.txt; tail -1 VYSTUP_test_hub6.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread $T -o $PRAC/test_hub6_tsan test_hub6.cpp && NAP_TSAN=1 TSAN_OPTIONS="halt_on_error=0" $PRAC/test_hub6_tsan > $PRAC/tsan_hub6.txt 2>&1
echo "== 6) priorita vlaken"; g++ -std=c++17 -O2 -pthread $T -o $PRAC/test_priorita test_priorita.cpp && $PRAC/test_priorita > VYSTUP_test_priorita.txt; tail -1 VYSTUP_test_priorita.txt
echo "== 7) JVM podpisy"
mkdir -p $PRAC/jvmlib $PRAC/jvmout
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $SEGA -o $PRAC/jvmlib/libnapsega_native_proof.so tv_sega6.cpp $CLOWN/*.o -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -o $PRAC/jvmlib/libnapps1core.so tv_ps1_6.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $INTRO -o $PRAC/jvmlib/libnapintro.so tv_intro.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2
javac -d $PRAC/jvmout -sourcepath jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NativeIntroBridge.java ../../app/src/main/java/eu/atarihelp/emu10/NativeSegaCoreBridge.java ../../app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java jvm/TestPodpisu.java 2>&1 | grep -v JAVA_TOOL
NAP_JVM_TEST=1 NAP_JVM_PNG=$PNG320 NAP_JVM_PRAC=$PRAC EGL_PLATFORM=surfaceless timeout 120 java -Djava.library.path=$PRAC/jvmlib -cp $PRAC/jvmout TestPodpisu 2>&1 | grep -v "JAVA_TOOL\|PIXFMT\|GLES_INIT\|BUILD2\|^DEMO\|^NAP\|CESTA_A" > jvm/VYSTUP_TestPodpisu.txt; tail -1 jvm/VYSTUP_TestPodpisu.txt
echo "== 8) B305 regrese (kodeky, pasy TSan, rozbocovac + TSan) a B303 Atari se strankou B306"
(cd ../b305 && T5="-I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb -I." && \
  g++ -std=c++17 -O2 -pthread -Wno-unused-function -Wno-missing-field-initializers $T5 -o $PRAC/test_kodeky test_kodeky.cpp && $PRAC/test_kodeky $PNG320 $PNG640 > $PRAC/kodeky.txt 2>&1; \
  g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -I../../app/src/main/cpp/tvproud -o $PRAC/test_pasy_tsan test_pasy.cpp && TSAN_OPTIONS="halt_on_error=0" $PRAC/test_pasy_tsan > $PRAC/pasy_tsan.txt 2>&1; \
  g++ -std=c++17 -O2 -pthread -Wno-unused-function $T5 -o $PRAC/test_hub test_hub.cpp && $PRAC/test_hub $PNG320 > $PRAC/hub.txt 2>&1; \
  g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $T5 -o $PRAC/test_hub_tsan test_hub.cpp && NAP_TSAN=1 TSAN_OPTIONS="halt_on_error=0" $PRAC/test_hub_tsan $PNG320 > $PRAC/hub_tsan.txt 2>&1)
# B303 test pise do ../b303/VYSTUP_test_b303_prohlizec.txt - vysledek se strankou B306 sem, original zpet
(cd ../b303 && g++ -std=c++17 -O2 -shared -fPIC -pthread -I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari -o libtvproud.so tv_lib.cpp && cp VYSTUP_test_b303_prohlizec.txt $PRAC/b303_puvodni.txt && timeout 600 python3 test_b303_prohlizec.py > $PRAC/b303_log.txt 2>&1; cp VYSTUP_test_b303_prohlizec.txt ../b306/VYSTUP_b303_atari_se_strankou_b306.txt; cp $PRAC/b303_puvodni.txt VYSTUP_test_b303_prohlizec.txt)
PRAC=$PRAC python3 - <<'EOF' > VYSTUP_tsan_a_regrese.txt
import os, re
S = os.environ['PRAC'] + '/'
def pocet(f):
    t = open(S + f, errors='replace').read(); return t.count('WARNING: ThreadSanitizer'), t
def mesa(t):
    # hlaseni, kde prvni ramec mimo TSan je uvnitr Mesy (libgallium / libEGL), se nepocitaji do naseho kodu
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
    return nase, m
def posl(t, n=1):
    r = t.strip().split('\n'); return r[-n] if len(r) >= n else ''
print('B306 - ThreadSanitizer a regrese B305 (vystupy z vse.sh)')
n, t = pocet('tsan_sega60.txt'); print('TSan, skutecne vlakno emulace Segy + jadro + rozbocovac + zvuk (test_sega_60): %d hlaseni; %s' % (n, posl(t)))
n, t = pocet('tsan_intro.txt'); a, b = mesa(t)
print('TSan, demo (GL vlakno, OpenSL zpetne volani, rozbocovac, test_intro_host): v nasem kodu %d, uvnitr Mesy (libgallium) %d; %s' % (a, b, posl(t, 2) if 'ThreadSanitizer: reported' in posl(t) else posl(t)))
n, t = pocet('tsan_hub6.txt'); print('TSan, rozbocovac - pakety jen se zvukem (test_hub6): %d hlaseni' % n)
print('B305 test_kodeky: %s' % posl(open(S + 'kodeky.txt').read()))
n, t = pocet('pasy_tsan.txt'); print('B305 test_pasy (koder JPEG na vic jadrech, TSan): %d hlaseni; %s' % (n, posl(t)))
print('B305 test_hub: %s' % posl(open(S + 'hub.txt').read()))
n, t = pocet('hub_tsan.txt'); print('B305 test_hub pod TSan: %d hlaseni; %s' % (n, posl(t)))
EOF
cat VYSTUP_tsan_a_regrese.txt; tail -1 VYSTUP_b303_atari_se_strankou_b306.txt
