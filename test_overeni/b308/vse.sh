#!/bin/bash
# B308: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b308)
# Spoustet odkudkoli: bash test_overeni/b308/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt):
#   PRAC     pracovni adresar (prelozene testy, logy)
#   INC      hlavicky EGL / GLES2 / KHR (jako ../b304),  JAVA_HOME = JDK
#   gcc, aarch64-linux-gnu-gcc + qemu-aarch64 (graficky cip jako v telefonu - NEON),
#   Chromium + Playwright pro Python, numpy, Pillow, knihovny Mesa (libEGL, libGLESv2)
set -u
PRAC=${PRAC:-/tmp/nap_b308}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b308
export EGL_PLATFORM=surfaceless
PS1="-DANDROID -I../b305/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux"
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }

echo "== 1) DOOM: skutecny graficky cip appky (gpulib + gpu_neon), x86 i arm64 (NEON pres qemu), s opravou i bez ni"
timeout 1800 python3 kontrola_doom.py $PRAC/doom 2>&1 | cisty > VYSTUP_kontrola_doom.txt; tail -1 VYSTUP_kontrola_doom.txt

echo "== 2) denik PS1, klavesnice z TV v C++, zamek kolem retro_run (skutecny nap_ps1_native.cpp)"
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wall -Wno-unused-function $PS1 -o $PRAC/libps1_b308.so ps1_harness.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
timeout 600 python3 test_ps1_denik.py $PRAC/libps1_b308.so $PRAC/denik 2>/dev/null | tail -1

echo "== 3) ThreadSanitizer: vlakno emulace + klavesy z prohlizece + denik + SAVE / LOAD + dotyk soubezne"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $PS1 -o $PRAC/tsan_denik tsan_denik.cpp \
    -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD_TSAN
rm -rf $PRAC/tsan_prac
TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 detect_deadlocks=0" timeout 900 $PRAC/tsan_denik $PRAC/tsan_prac > $PRAC/tsan_denik.txt 2>&1
{
  echo "B308 ThreadSanitizer - SKUTECNY nap_ps1_native.cpp: vlakno emulace, zvukova karta (OpenSL), 2 vlakna"
  echo "prohlizece s klavesami, stranka cte denik, SAVE / LOAD pozice, dotyk + zmeny obrazu + hlasky jadra, 7 s"
  grep "^start\|^stop\|^bios\|^denik" $PRAC/tsan_denik.txt | cisty
  echo "hlaseni ThreadSanitizer: $(grep -c 'WARNING: ThreadSanitizer' $PRAC/tsan_denik.txt)"
} > VYSTUP_tsan_denik.txt; tail -1 VYSTUP_tsan_denik.txt

echo "== 4) JVM: skutecna trida NativePs1CoreBridge (ps1PcKlavesa, ps1Denik, obaly) se skutecnym C++, -Xcheck:jni"
mkdir -p $PRAC/jvmlib $PRAC/jvmout; rm -rf $PRAC/jvm_prac
cp $PRAC/libps1_b308.so $PRAC/jvmlib/libnapps1core.so
javac -d $PRAC/jvmout -sourcepath ../b306/jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java jvm/TestPodpisu.java 2>&1 | grep -v JAVA_TOOL
timeout 120 java -Xcheck:jni -Djava.library.path=$PRAC/jvmlib -cp $PRAC/jvmout TestPodpisu $PRAC/jvm_prac 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$" | cisty > jvm/VYSTUP_TestPodpisu.txt; tail -1 jvm/VYSTUP_TestPodpisu.txt
echo "   vsechny native metody tridy maji v knihovne svou funkci:"
{
  for m in $(grep -oE "native [][a-zA-Z]+ [a-zA-Z0-9_]+\(" ../../app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java | awk '{print $3}' | tr -d '('); do
    if nm -D --defined-only $PRAC/libps1_b308.so | grep -q " Java_eu_atarihelp_emu10_NativePs1CoreBridge_${m}$"; then echo "OK     $m"; else echo "CHYBA  $m (v knihovne neni)"; fi
  done
} > jvm/VYSTUP_symboly.txt; echo "   $(grep -c '^OK' jvm/VYSTUP_symboly.txt) OK, $(grep -c '^CHYBA' jvm/VYSTUP_symboly.txt) CHYB"

echo "== 5) klavesnice PC v TV prohlizeci -> PS1 (Chromium, stranka z MainActivity.java)"
timeout 600 python3 test_b308_klavesnice.py $PRAC/libps1_b308.so $PRAC/klavesnice 2>/dev/null | tail -1

echo "== 6) stranka PS1: nahravani hry podle skutecneho stavu, banner mimo monitor, LOG s denikem (Chromium)"
timeout 600 python3 test_b308_ps1_stranka.py 2>/dev/null | tail -1

echo "== 7) regrese: Atari 130XE (C++) - klavesnice a vkladani v prohlizeci se strankou B308"
(cd ../b303 && g++ -std=c++17 -O2 -shared -fPIC -pthread -I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari \
    -o libtvproud.so tv_lib.cpp && cp VYSTUP_test_b303_prohlizec.txt $PRAC/b303_puvodni.txt && \
    timeout 600 python3 test_b303_prohlizec.py > $PRAC/b303_log.txt 2>&1; \
    cp VYSTUP_test_b303_prohlizec.txt ../b308/VYSTUP_b303_atari_se_strankou_b308.txt; \
    cp $PRAC/b303_puvodni.txt VYSTUP_test_b303_prohlizec.txt; rm -f libtvproud.so)
tail -1 VYSTUP_b303_atari_se_strankou_b308.txt
