#!/bin/bash
# B311: vsechny testy hlavniho menu najednou (vystupy VYSTUP_* sem do test_overeni/b311)
# Spoustet odkudkoli: PRAC=... INC=... GLSLANG=... ANDROID_JAR=... bash test_overeni/b311/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): g++, JAVA_HOME = JDK (javac, hlavicky JNI), hlavicky
# EGL / GLES2 / KHR v $INC (jako ../b304), knihovny Mesa (libEGL, libGLESv2), Python 3 + Pillow,
# Chromium + Playwright pro Python, ffmpeg / ffprobe, glslangValidator ($GLSLANG),
# android.jar API 34 ($ANDROID_JAR - jen pro preklad Javy proti skutecnemu Androidu).
set -u
PRAC=${PRAC:-/tmp/nap_b311}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b311
export EGL_PLATFORM=surfaceless
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
JNI="-I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud"

echo "== 1) C++ logika karuselu: karty.txt (i chybne radky), fyzika prstu a svihu, rozlozeni, klavesy"
g++ -std=c++17 -O2 -I../../app/src/main/cpp/menu -o $PRAC/test_karusel test_karusel.cpp || echo CHYBA_PREKLAD
$PRAC/test_karusel > VYSTUP_test_karusel.txt 2>&1; tail -1 VYSTUP_test_karusel.txt

echo "== 2) snimky menu bez telefonu (Mesa): na vysku, zamky, na sirku, TV 854x480, rychlost"
g++ -std=c++17 -O2 -w -I$INC -o $PRAC/menu_snimky menu_snimky.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
mkdir -p $PRAC/snimky
{ echo "B311 - snimky hlavniho menu pres Mesa (skutecny C++ z app/src/main/cpp/menu); PNG v PRAC/snimky"
  timeout 900 $PRAC/menu_snimky $PRAC/snimky 1080 2340 0:0.35 0:3 1:3 3:3 4:3 5:3 6:3 2.5:3
  timeout 600 $PRAC/menu_snimky $PRAC/snimky 1080 2340 --zamky 1:3 2:3
  timeout 600 $PRAC/menu_snimky $PRAC/snimky 2340 1080 2:3 5:3
  timeout 600 $PRAC/menu_snimky $PRAC/snimky 854 480 --tv 0:3
  timeout 600 $PRAC/menu_snimky $PRAC/snimky 1080 2340 --vykon 6; } 2>&1 | grep -v "^karta \|^DEMO" | cisty > VYSTUP_menu_snimky.txt
grep "CHYB GL celkem\|VYKON" VYSTUP_menu_snimky.txt | sort | uniq -c

echo "== 3) SKUTECNY nap_menu_gl.cpp jako z Javy (JNI): karty, plocha, dotyk, klavesy, proud na TV, plocha pryc / zpet"
g++ -std=c++17 -O2 -pthread -w $JNI -o $PRAC/test_menu_host test_menu_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
timeout 600 $PRAC/test_menu_host 2>&1 | cisty > VYSTUP_test_menu_host.txt; tail -1 VYSTUP_test_menu_host.txt

echo "== 4) ThreadSanitizer: totez pod -fsanitize=thread (zavody v nasem kodu = 0)"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -w $JNI -o $PRAC/test_menu_host_tsan test_menu_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0" timeout 1800 $PRAC/test_menu_host_tsan > $PRAC/tsan_menu.txt 2>&1
python3 tsan_rozbor.py $PRAC/tsan_menu.txt | tail -1

echo "== 5) shadery menu prisne podle GLSL ES 1.00 (glslangValidator), vcetne spojeni programu"
python3 kontrola_shaderu.py > /dev/null; tail -1 VYSTUP_kontrola_shaderu.txt

echo "== 6) hlavni menu v Chromiu: zaloha v HTML, pod C++ menu, OPTIONS, adresa WEB TV"
timeout 600 python3 test_b311_menu.py $PRAC/menu > /dev/null 2>&1; tail -1 VYSTUP_test_b311_menu.txt

echo "== 7) uplny navod v Chromiu: obsah, kapitoly, hledani, historie, rozvrzeni 320 / 360 / na sirku"
timeout 600 python3 test_b311_navod.py $PRAC/navod > /dev/null 2>&1; tail -1 VYSTUP_test_b311_navod.txt
python3 kontrola_navodu.py > /dev/null; tail -1 VYSTUP_kontrola_navodu.txt

echo "== 8) DZUBOX: RECORD ulozi nahravku (po kouscich do Javy), chyba zapisu, prohlizec na PC"
timeout 600 python3 test_b311_dj_nahravka.py $PRAC/dj > /dev/null 2>&1; tail -1 VYSTUP_test_b311_dj_nahravka.txt

echo "== 9) menu na TV / PC: proud z C++ (/menu.stream) v prohlizeci + klavesnice PC"
g++ -std=c++17 -O2 -shared -fPIC -pthread -w $JNI -o $PRAC/libmenu_b311.so tv_menu_lib.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
timeout 900 python3 test_b311_tv_menu.py $PRAC $PRAC/tv > /dev/null 2>&1; tail -1 VYSTUP_test_b311_tv_menu.txt

echo "== 10) mosty JavaScript -> Java a odkazy na soubory"
python3 kontrola_mostu.py > VYSTUP_kontrola_mostu.txt 2>&1; tail -1 VYSTUP_kontrola_mostu.txt
python3 kontrola_odkazu.py > /dev/null; tail -1 VYSTUP_kontrola_odkazu.txt

echo "== 11) Java appky prelozena proti SKUTECNEMU Androidu (android.jar API 34, Java 17 jako v build.gradle)"
if [ -n "${ANDROID_JAR:-}" ] && [ -f "$ANDROID_JAR" ]; then
  rm -rf $PRAC/javac_out && mkdir -p $PRAC/javac_out
  { echo "B311 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    (cd $R && timeout 900 javac -proc:none --release 17 -cp "$ANDROID_JAR" -Xmaxerrs 1000 -nowarn -d $PRAC/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -v JAVA_TOOL)
    echo "trid prelozeno: $(find $PRAC/javac_out -name '*.class' | wc -l)"
    n=$(cd $R && javac -proc:none --release 17 -cp "$ANDROID_JAR" -nowarn -d $PRAC/javac_out app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -c "error:")
    echo "VYSLEDEK: $n chyb prekladu"; } > VYSTUP_javac_android.txt
  tail -1 VYSTUP_javac_android.txt
else
  echo "   (preskoceno - ANDROID_JAR neni nastaveny)"
fi

echo "== 11b) podpisy JNI: Java NativeMenuBridge (javac -h) proti C++ nap_menu_gl.cpp, export funkci z knihovny"
if [ -n "${ANDROID_JAR:-}" ] && [ -f "$ANDROID_JAR" ]; then
  rm -rf $PRAC/jnih && mkdir -p $PRAC/jnih/h $PRAC/jnih/out
  (cd $R && javac -proc:none --release 17 -cp "$ANDROID_JAR" -h $PRAC/jnih/h -d $PRAC/jnih/out app/src/main/java/eu/atarihelp/emu10/NativeMenuBridge.java 2>&1 | grep -v JAVA_TOOL)
  python3 kontrola_jni.py $PRAC/jnih/h/eu_atarihelp_emu10_NativeMenuBridge.h | tail -1
fi
g++ -std=c++17 -O2 -fPIC -fvisibility=hidden -w -c $JNI -I../../app/src/main/cpp/menu -I../../app/src/main/cpp/intro -o $PRAC/nap_menu_gl.o ../../app/src/main/cpp/menu/nap_menu_gl.cpp || echo CHYBA_PREKLAD
echo "   JNI funkci viditelnych z knihovny (-fvisibility=hidden, JNIEXPORT): $(readelf -sW $PRAC/nap_menu_gl.o | grep ' GLOBAL DEFAULT .* Java_eu_atarihelp_emu10_NativeMenuBridge_' | wc -l) z 22" | tee -a VYSTUP_kontrola_jni.txt

echo "== 12) regrese TV / PC v Chromiu se strankou z MainActivity B311: klavesnice PC (B302), Atari na TV + Ctrl+V (B303)"
ATARI="-I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari"
(cd ../b302 && cp VYSTUP_test_b302_prohlizec.txt $PRAC/b302_puvodni.txt && \
  g++ -std=c++17 -O2 -shared -fPIC -I../../app/src/main/cpp/atari -o libpcklav.so pc_klav_lib.cpp && \
  timeout 900 python3 test_b302_prohlizec.py > $PRAC/b302_log.txt 2>&1; \
  cp VYSTUP_test_b302_prohlizec.txt ../b311/VYSTUP_b302_klavesnice_pc_se_strankou_b311.txt; \
  cp $PRAC/b302_puvodni.txt VYSTUP_test_b302_prohlizec.txt; rm -f libpcklav.so)
tail -1 VYSTUP_b302_klavesnice_pc_se_strankou_b311.txt
(cd ../b303 && cp VYSTUP_test_b303_prohlizec.txt $PRAC/b303_puvodni.txt && \
  g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $ATARI -o libtvproud.so tv_lib.cpp && \
  timeout 900 python3 test_b303_prohlizec.py > $PRAC/b303_log.txt 2>&1; \
  cp VYSTUP_test_b303_prohlizec.txt ../b311/VYSTUP_b303_atari_tv_se_strankou_b311.txt; \
  cp $PRAC/b303_puvodni.txt VYSTUP_test_b303_prohlizec.txt; rm -f libtvproud.so)
tail -1 VYSTUP_b303_atari_tv_se_strankou_b311.txt
