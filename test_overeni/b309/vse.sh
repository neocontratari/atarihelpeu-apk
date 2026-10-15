#!/bin/bash
# B309: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b309)
# Spoustet odkudkoli: bash test_overeni/b309/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt):
#   PRAC     pracovni adresar (prelozene testy, snimky, logy)
#   INC      hlavicky EGL / GLES2 / KHR (jako ../b304),  JAVA_HOME = JDK
#   PNG320   snimky 3D sceny 320x240 (../b305/JAK_SPUSTIT.txt krok 0), CLOWN = objekty jadra clownmdemu *.o
#   HRA      (nepovinne) Reneho Pitt-Kitt Platinum .xex - overi, ze novy klip v assets vznikl z hry
#   Chromium + Playwright pro Python, numpy, Pillow, knihovny Mesa (libEGL, libGLESv2), zlib
set -u
PRAC=${PRAC:-/tmp/nap_b309}
INC=${INC:-$PRAC/inc}
PNG320=${PNG320:-$PRAC/ps1_320}
CLOWN=${CLOWN:-$PRAC/clown}
HRA=${HRA:-}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b309
export EGL_PLATFORM=surfaceless
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
V=../../app/src/main/cpp/vendor/clownmdemu-core
SEGA="-DNAP_SEGA_VENDOR_CORE_PRESENT=1 -DCC_USE_C99_INTEGERS=1 -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clownz80/source"
PS1="-DANDROID -I../b305/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/vendor/stb"
INTRO="-I../b306/stub -I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/intro"

echo "== 1) TV / PC: prepnuti hry ci emulatoru bez F5 (Chromium, H.264 cesta jako u Reneho, proud z C++)"
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -o $PRAC/libps1_b309.so ../b306/tv_ps1_6.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PS1
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $SEGA -o $PRAC/libsega_b309.so ../b306/tv_sega6.cpp $CLOWN/*.o -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_SEGA
timeout 600 python3 test_b309_prepnuti.py $PNG320 $PRAC $PRAC/prepnuti > $PRAC/log_prepnuti.txt 2>&1; tail -1 VYSTUP_test_b309_prepnuti.txt
echo "   stejny scenar se strankou z B308 (commit 978bee1) - tam ma byt Reneho chyba videt:"
timeout 600 python3 test_b309_prepnuti.py $PNG320 $PRAC $PRAC/prepnuti_b308 --ref 978bee1 > $PRAC/log_prepnuti_b308.txt 2>&1; tail -1 VYSTUP_test_b309_prepnuti_978bee1.txt

echo "== 2) PS1: ULOZIT LOG A ODESLAT (Chromium, stranka PS1)"
timeout 300 python3 test_b309_ps1_log.py > /dev/null 2>&1; tail -1 VYSTUP_test_b309_ps1_log.txt

echo "== 3) uvodni demo 59 s: cele po 0,25 s na vysku (540x1170) a na sirku / TV (854x480)"
g++ -std=c++17 -O2 -Wno-unused-function -I$INC -o $PRAC/demo_snimky ../b307/demo_snimky.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz || echo CHYBA_PREKLAD
rm -rf $PRAC/vse_v $PRAC/vse_s; mkdir -p $PRAC/vse_v $PRAC/vse_s
$PRAC/demo_snimky $PRAC/vse_v 540 1170 --vse 0.25 > $PRAC/vse_v.txt 2>&1
$PRAC/demo_snimky $PRAC/vse_s 854 480 --vse 0.25 > $PRAC/vse_s.txt 2>&1
{
  echo "B309 - cele demo (59 s) po 0,25 s, stejny C++ jako v telefonu (Mesa, OpenGL ES 2.0 bez okna)"
  for f in vse_v vse_s; do
    echo "=== $( [ $f = vse_v ] && echo 'telefon na vysku 540x1170' || echo 'na sirku / TV 854x480' ) ==="
    grep -v "^t=\|^DEMO" $PRAC/$f.txt | cisty
    echo "snimku: $(grep -c '^t=' $PRAC/$f.txt), chyb GL: $(grep -c 'GL chyba' $PRAC/$f.txt)"
    echo "defile a logo (32,5 - 59 s):"
    grep "^t=" $PRAC/$f.txt | awk '$2>=32.49 && $2<=59.01' | cisty
  done
} > VYSTUP_demo_snimky.txt
grep "^snimku" VYSTUP_demo_snimky.txt

echo "== 4) zvuk dema 59,6 s (stopa + zvuk hry Pitt-Kitt)"
g++ -std=c++17 -O2 -Wno-unused-function -o $PRAC/demo_zvuk demo_zvuk.cpp && $PRAC/demo_zvuk $PRAC/demo.wav 2>&1 | cisty > VYSTUP_demo_zvuk.txt; head -3 VYSTUP_demo_zvuk.txt

echo "== 5) tlacitko SKIP (cele demo, po 58,6 s neni)"
g++ -std=c++17 -O2 -Wno-unused-function -I$INC -o $PRAC/test_skip test_skip.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz && $PRAC/test_skip 2>&1 | grep -v "^DEMO" > VYSTUP_test_skip.txt; tail -1 VYSTUP_test_skip.txt

echo "== 6) SKUTECNY nap_intro_gl.cpp na PC (OpenSL, dotyk, CELA stopa 59,6 s bit po bitu, zvuk hry)"
mkdir -p $PRAC/assety_bez_klipu/design
ln -sf $R/app/src/main/assets/design/nap_logo_original.png $PRAC/assety_bez_klipu/design/nap_logo_original.png
g++ -std=c++17 -O2 -pthread -Wno-unused-function $INTRO -o $PRAC/test_intro_host test_intro_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 && \
  timeout 600 $PRAC/test_intro_host $PRAC/assety_bez_klipu 2>&1 | grep -v "^DEMO\|^NAP_DEMO" | cisty > VYSTUP_test_intro_host.txt; tail -1 VYSTUP_test_intro_host.txt

echo "== 7) prohlizec (Chromium): demo 59 s na TV / PC proudem z C++ (Pitt-Kitt, 16 bit, 32 bit, zvuk bit po bitu)"
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $INTRO -o $PRAC/libintro_b309.so tv_intro.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2
PRAC=$PRAC timeout 400 python3 test_b309_intro_prohlizec.py > $PRAC/log_intro_prohlizec.txt 2>&1; tail -1 VYSTUP_test_b309_intro_prohlizec.txt

echo "== 8) 16bit ukazka 6 s (snimky 0..420): rychlost"
g++ -std=c++17 -O2 -Wno-unused-function -o $PRAC/ukazka16 ukazka16.cpp && mkdir -p $PRAC/u16 && $PRAC/ukazka16 $PRAC/u16 > $PRAC/u16.txt 2>&1
{ echo "B309 - 16bit ukazka (procesor, 320x224, jedno jadro PC), snimky 0..420 (ukazka 6 s = 36..396)"; sed 's/^/  /' $PRAC/u16.txt; } > VYSTUP_ukazka16.txt; tail -1 VYSTUP_ukazka16.txt

if [ -n "$HRA" ] && [ -f "$HRA" ]; then
  echo "== 9) klip Pitt-Kitt znovu z Reneho hry (SKUTECNE jadro Atari 130XE z appky)"
  g++ -std=c++17 -O2 -Wno-unused-function -Wno-unused-result -I../../app/src/main/cpp/atari -o $PRAC/nahraj_hru ../b307/nahraj_hru.cpp -lz
  $PRAC/nahraj_hru "$HRA" zaznam $PRAC/klip_znovu.bin "110-189,232-289,438-615" "200-206:START,420-426:START" > $PRAC/nahraj.txt 2>&1
  { echo "B309 - klip intro/pittkitt.klip znovu nahrany z Reneho Pitt-Kitt Platinum (.xex):"
    echo "  useky: titulek N&P 110-189, kapitola Las Vegas 1982 232-289, jizda 438-615 (do 615 - pak hra restartuje kolo)"
    sed "s#$PRAC/##" $PRAC/nahraj.txt
    if cmp -s $PRAC/klip_znovu.bin ../../app/src/main/assets/intro/pittkitt.klip; then
      echo "VYSLEDEK: klip v assets je bajt po bajtu stejny (sha256 $(sha256sum $PRAC/klip_znovu.bin | cut -c1-16)...)"
    else echo "VYSLEDEK: CHYBA - klip v assets se lisi"; fi; } > VYSTUP_klip.txt
  tail -1 VYSTUP_klip.txt
fi
