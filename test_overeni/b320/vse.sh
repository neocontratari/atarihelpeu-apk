#!/bin/bash
# B320: HERNI OVLADAC (Retroid Pocket 6, Bluetooth / USB gamepad) pro PS1, Segu, Atari a menu + PS1 NA TV / PC
# S DITHERINGEM (Dune 2000) + STRANKA WEB TV SE PO AKTUALIZACI APPKY OBNOVI SAMA. Vsechny testy najednou
# (vystupy VYSTUP_* sem). Spoustet odkudkoli:
#   PRAC=... INC=... ANDROID_JAR=... PNG320=... PNG640=... bash test_overeni/b320/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): g++ / gcc, clang + ld.lld + llvm-objcopy (PS-EXE pro mipsel),
# JAVA_HOME = JDK, hlavicky EGL / GLES2 / KHR v $INC (napr. balicky libegl-dev libgles-dev -> /usr/include),
# knihovny Mesa (libEGL, libGLESv2), zlib, Python 3 + Playwright + Chromium + numpy, Node.js (acorn),
# android.jar API 34 ($ANDROID_JAR), snimky 320x240 a 640x480 ($PNG320, $PNG640 - ../b305/JAK_SPUSTIT.txt
# krok 0), git.
set -u
PRAC=${PRAC:-/tmp/nap_b320}
INC=${INC:-/usr/include}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
AJAR=${ANDROID_JAR:-}
PNG320=${PNG320:-$PRAC/png320}
PNG640=${PNG640:-$PRAC/png640}
R=$(cd "$(dirname "$0")/../.." && pwd)
Z=$R/test_overeni/b320
mkdir -p $PRAC
cd $Z
export EGL_PLATFORM=surfaceless
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
TVP="-I$R/app/src/main/cpp/tvproud -I$R/app/src/main/cpp/vendor/stb -I../b319"
PS1="-DANDROID -I../b305/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux"

echo "== 1) kodek PS1 proudu s ditheringem (synteticka Dune 2000 2x / 1x, 3D hra, hra bez ditheringu = B319)"
g++ -std=c++17 -O2 -pthread -Wall $TVP -o $PRAC/test_b320_kodek test_b320_kodek.cpp 2> $PRAC/preklad_kodek.txt || echo CHYBA_PREKLAD
$PRAC/test_b320_kodek | cisty > VYSTUP_test_b320_kodek.txt; tail -1 VYSTUP_test_b320_kodek.txt

echo "== 2) SKUTECNY vykreslovac PS1 z appky (gpulib + gpu_neon, 2x Enhanced Resolution, dithering) + jadro PCSX + nap_ps1_native.cpp"
[ -n "$(ls $PRAC/jadro/*.o 2>/dev/null)" ] || bash ../b318/stavba_jadra.sh $PRAC > $PRAC/stavba_jadra.txt 2>&1
[ -n "$(ls $PRAC/vykreslovac/*.o 2>/dev/null)" ] || bash stavba_vykreslovace.sh $PRAC > $PRAC/stavba_vykreslovace.txt 2>&1
python3 udelej_mapu.py $PRAC > $PRAC/mapa.txt 2>&1 || echo CHYBA_PSEXE
g++ -std=c++17 -O2 -g -pthread $PS1 -I$R/app/src/main/cpp/vendor/stb -o $PRAC/integrace_b320_gpu integrace_b320_gpu.cpp \
    $PRAC/jadro/*.o $PRAC/vykreslovac/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl 2> $PRAC/preklad_gpu.txt || echo CHYBA_PREKLAD
(cd $PRAC && timeout 900 ./integrace_b320_gpu $PRAC $PRAC/mapa_dune.exe > integrace_b320_gpu.txt 2>&1)
grep -v "PIXFMT_CHECK\|^BUILD2S\|^CESTA\|^\[\|^$" $PRAC/integrace_b320_gpu.txt | cisty > VYSTUP_integrace_b320_gpu.txt
tail -1 VYSTUP_integrace_b320_gpu.txt

echo "== 3) herni ovladac - logika (nap_ovladac.h + mys nap_ps1_mys.h): tlacitka podle mista, osy, packy, mys packou"
g++ -std=c++17 -O2 -Wall -Wextra -pthread -I$R/app/src/main/cpp -o $PRAC/test_b320_ovladac test_b320_ovladac.cpp || echo CHYBA_PREKLAD
$PRAC/test_b320_ovladac ../.. | cisty > VYSTUP_test_b320_ovladac.txt; tail -1 VYSTUP_test_b320_ovladac.txt

echo "== 4) herni ovladac v PS1: SKUTECNE jadro PCSX + nap_ps1_native.cpp (hra mys_test.exe) + ThreadSanitizer"
[ -f $PRAC/mys_test.exe ] || (cd ../b318 && python3 udelej_exe.py $PRAC > /dev/null 2>&1)
g++ -std=c++17 -O1 -g -pthread $PS1 -o $PRAC/integrace_b320_ovladac integrace_b320_ovladac.cpp $PRAC/jadro/*.o \
    -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl 2> $PRAC/preklad_ovl.txt || echo CHYBA_PREKLAD
(cd $PRAC && timeout 900 ./integrace_b320_ovladac $PRAC $PRAC/mys_test.exe > integrace_b320_ovladac.txt 2>&1)
grep -E "^(OK|CHYBA|VYSLEDEK)" $PRAC/integrace_b320_ovladac.txt | cisty > VYSTUP_integrace_b320_ovladac.txt
tail -1 VYSTUP_integrace_b320_ovladac.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread $PS1 -o $PRAC/integrace_b320_ovladac_tsan integrace_b320_ovladac.cpp $PRAC/jadro/*.o \
    -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl 2> $PRAC/preklad_ovl_tsan.txt || echo CHYBA_PREKLAD_TSAN
(cd $PRAC && NAP_TSAN=1 TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 detect_deadlocks=0" timeout 1200 \
    ./integrace_b320_ovladac_tsan $PRAC $PRAC/mys_test.exe > tsan_ovladac.txt 2>&1)
{ grep -E "^(OK|CHYBA|VYSLEDEK)" $PRAC/tsan_ovladac.txt | cisty
  echo "hlaseni ThreadSanitizer: $(grep -c 'WARNING: ThreadSanitizer' $PRAC/tsan_ovladac.txt)"; } > VYSTUP_tsan_b320_ovladac.txt
tail -1 VYSTUP_tsan_b320_ovladac.txt

echo "== 5) herni ovladac v Atari 130XE (skutecny nap_atari_native.cpp): joystick, konzole, MEZERA / RETURN, D-pad na sirku"
g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JV/include -I$JV/include/linux -I$R/app/src/main/cpp/atari \
    -o $PRAC/test_b320_atari_ovladac test_b320_atari_ovladac.cpp 2> $PRAC/preklad_atari.txt || echo CHYBA_PREKLAD
(cd $PRAC && timeout 300 ./test_b320_atari_ovladac > atari_ovladac.txt 2>&1)
grep -v "^20[0-9][0-9]-" $PRAC/atari_ovladac.txt | cisty > VYSTUP_test_b320_atari_ovladac.txt; tail -1 VYSTUP_test_b320_atari_ovladac.txt

echo "== 6) herni ovladac na Seze (skutecny nap_sega_native_proof.cpp, jadro clownmdemu se pta na tlacitka)"
V=$R/app/src/main/cpp/vendor/clownmdemu-core
CLOWN=$PRAC/clown
if [ ! -f $CLOWN/hotovo ]; then
  mkdir -p $CLOWN
  CF="-O2 -fPIC -w -DCC_USE_C99_INTEGERS=1 -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clown68000/source/common -I$V/libraries/clown68000/libraries/clowncommon -I$V/libraries/clownz80/source -I$V/libraries/clownz80/libraries/clowncommon"
  for f in bus-common bus-main-m68k bus-sub-m68k bus-z80 cdc cdda clownmdemu controller controller-manager controller-multitap-ea \
           controller-multitap-sega fm fm-channel fm-lfo fm-operator fm-phase io-port log low-pass-filter pcm psg sync vdp; do
    gcc $CF -c $V/source/$f.c -o $CLOWN/$f.o || echo CHYBA_CLOWN $f
  done
  gcc $CF -c $V/libraries/clown68000/source/common/opcode.c -o $CLOWN/m68k_opcode.o
  gcc $CF -c $V/libraries/clown68000/source/interpreter/clown68000.c -o $CLOWN/m68k_clown68000.o
  gcc $CF -c $V/libraries/clownz80/source/common.c -o $CLOWN/z80_common.o
  gcc $CF -c $V/libraries/clownz80/source/interpreter.c -o $CLOWN/z80_interpreter.o
  touch $CLOWN/hotovo
fi
SEGA="-DNAP_SEGA_VENDOR_CORE_PRESENT=1 -DCC_USE_C99_INTEGERS=1 -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clownz80/source"
g++ -std=c++17 -O2 -pthread -Wno-unused-function $SEGA -o $PRAC/test_b320_sega_ovladac test_b320_sega_ovladac.cpp $CLOWN/*.o \
    -l:libEGL.so.1 -l:libGLESv2.so.2 2> $PRAC/preklad_sega.txt || echo CHYBA_PREKLAD
$PRAC/test_b320_sega_ovladac 2>&1 | grep -E "^(B320|OK|CHYBA|VYSLEDEK)" | cisty > VYSTUP_test_b320_sega_ovladac.txt
tail -1 VYSTUP_test_b320_sega_ovladac.txt

echo "== 7) JVM: NapOvladac (ciste casti bez Androidu) + NativePs1CoreBridge se skutecnym nap_ps1_native.cpp (-Xcheck:jni)"
J=$PRAC/jvm320; mkdir -p $J/lib $J/out $J/prac $Z/jvm
(cd ../b308 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wall -Wno-unused-function $PS1 -o $J/lib/libnapps1core.so ps1_harness.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2) 2> $PRAC/preklad_jvm.txt || echo CHYBA_PREKLAD
if [ -n "$AJAR" ] && [ -f "$AJAR" ]; then
  A=$R/app/src/main/java/eu/atarihelp/emu10
  $JV/bin/javac -proc:none --release 17 -cp $AJAR -d $J/out $A/NapOvladac.java $A/NativePs1CoreBridge.java $A/NativeSegaCoreBridge.java \
      $A/NativeAtariCoreBridge.java jvm/eu/atarihelp/emu10/TestB320Ovladac.java 2>&1 | grep -v "JAVA_TOOL\|^Note"
  # (na konci "terminate called" = vlakno zvukove karty v nahrade OpenSL testu B308 - ne appka)
  (cd $J && timeout 120 $JV/bin/java -Xcheck:jni -Djava.library.path=$J/lib -cp $J/out:$AJAR eu.atarihelp.emu10.TestB320Ovladac $J/prac 2>&1) \
      | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$\|^BUILD2S\|PIXFMT\|JAVA_TOOL\|^terminate called" | cisty > jvm/VYSTUP_TestB320Ovladac.txt
  tail -1 jvm/VYSTUP_TestB320Ovladac.txt
else
  echo "(JVM preskoceno - ANDROID_JAR neni)"
fi

echo "== 8) stranky PS1, Sega a OPTIONS s hernim ovladacem (Chromium, stranky presne z appky)"
timeout 900 python3 test_b320_stranky.py > $PRAC/stranky.txt 2>&1; tail -1 VYSTUP_test_b320_stranky.txt

echo "== 9) stranka WEB TV se po aktualizaci appky obnovi sama (Chromium, stranka z MainActivity)"
timeout 900 python3 test_b320_tv_verze.py > $PRAC/tv_verze.txt 2>&1; tail -1 VYSTUP_test_b320_tv_verze.txt

echo "== 10) regrese (vse z B319 + testy B319 na kodu B320, javac, podpisy JNI 3 mostu, stranka WEB TV starsich testu)"
bash regrese.sh $PRAC $INC $JV "$AJAR" $PNG320 $PNG640 > /dev/null 2>&1; tail -1 VYSTUP_regrese.txt
