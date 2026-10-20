#!/bin/bash
# B313: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b313)
# Spoustet odkudkoli: PRAC=... INC=... GLSLANG=... ANDROID_JAR=... bash test_overeni/b313/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): g++, JAVA_HOME = JDK (javac, hlavicky JNI), hlavicky
# EGL / GLES2 / KHR + stb_image_write.h v $INC (jako ../b311), knihovny Mesa (libEGL, libGLESv2),
# Python 3 + numpy + Pillow, Chromium + Playwright pro Python, glslangValidator ($GLSLANG),
# android.jar API 34 ($ANDROID_JAR - jen pro preklad Javy proti skutecnemu Androidu).
set -u
PRAC=${PRAC:-/tmp/nap_b313}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b313
export EGL_PLATFORM=surfaceless
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
JNI="-I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud"
TV="-I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb"

echo "== 1) postup se po nove instalaci nevraci: zaloha Androidu vypnuta (allowBackup=false), log pri startu"
python3 kontrola_zalohy.py > /dev/null; tail -1 VYSTUP_kontrola_zalohy.txt

echo "== 2) TV / PC: prohlizec se po odchodu z menu prepne na spravny proud (409 + X-Nap-Proud, Chromium)"
timeout 600 python3 test_b313_tv_prepnuti.py > /dev/null 2>&1; tail -1 VYSTUP_test_b313_tv_prepnuti.txt

echo "== 3) kodek JPEG pro menu na TV: vic obdelniku, zjemneni za pohybu, kvalita, vlakna (+ ThreadSanitizer)"
g++ -std=c++17 -O2 -pthread -w $TV -o $PRAC/test_kodek_menu test_kodek_menu.cpp || echo CHYBA_PREKLAD
$PRAC/test_kodek_menu > VYSTUP_test_kodek_menu.txt 2>&1; tail -1 VYSTUP_test_kodek_menu.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -w $TV -o $PRAC/test_kodek_menu_tsan test_kodek_menu.cpp || echo CHYBA_PREKLAD
TSAN_OPTIONS="halt_on_error=0" timeout 1200 $PRAC/test_kodek_menu_tsan > $PRAC/kodek_tsan.txt 2>&1
{ echo "B313 - test_kodek_menu pod ThreadSanitizer (-O1 -g -fsanitize=thread)"; grep "VYSLEDEK" $PRAC/kodek_tsan.txt
  echo "hlaseni ThreadSanitizeru: $(grep -c 'WARNING: ThreadSanitizer' $PRAC/kodek_tsan.txt)"; } > VYSTUP_test_kodek_menu_tsan.txt
tail -1 VYSTUP_test_kodek_menu_tsan.txt

echo "== 4) SKUTECNY nap_menu_gl.cpp jako z Javy (JNI): velikost TV podle kvality, menuGrabFrame, perioda displeje"
g++ -std=c++17 -O2 -pthread -w $JNI -o $PRAC/test_menu_host_b313 test_menu_host_b313.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
timeout 600 $PRAC/test_menu_host_b313 2>&1 | grep -v "^karta \|^DEMO\|^MENU" | cisty > VYSTUP_test_menu_host_b313.txt; tail -1 VYSTUP_test_menu_host_b313.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -w $JNI -o $PRAC/test_menu_host_b313_tsan test_menu_host_b313.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0" timeout 1800 $PRAC/test_menu_host_b313_tsan > $PRAC/tsan_menu.txt 2>&1
python3 tsan_rozbor.py $PRAC/tsan_menu.txt | tail -1

echo "== 5) menu na TV / PC v Chromiu: 1280x720, OSTRE v klidu (canvas proti obrazu menu), zmena kvality, klavesnice"
g++ -std=c++17 -O2 -shared -fPIC -pthread -w $JNI -o $PRAC/libmenu_b313.so tv_menu_lib_b313.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
timeout 900 python3 test_b313_tv_menu.py $PRAC $PRAC/tv > /dev/null 2>&1; tail -1 VYSTUP_test_b313_tv_menu.txt

echo "== 6) mereni: skutecne menu (plocha jako S8 1080x2004) na TV 1280x720 - kB na snimek, casti, ostrost"
g++ -std=c++17 -O2 -pthread -w $JNI -o $PRAC/mereni_menu_tv mereni_menu_tv.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
mkdir -p $PRAC/mereni
timeout 600 $PRAC/mereni_menu_tv $PRAC/mereni 2 2>&1 | grep -v "^karta \|^DEMO\|^MENU" | cisty > VYSTUP_mereni_menu_tv.txt; grep "klid 2\|listovani " VYSTUP_mereni_menu_tv.txt

echo "== 7) konzole N&P 16 / N&P 32: animace (logika) a obrazky karty (Mesa)"
g++ -std=c++17 -O2 -w -I$INC -o $PRAC/test_konzole_anim test_konzole_anim.cpp -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
$PRAC/test_konzole_anim > VYSTUP_test_konzole_anim.txt 2>&1; tail -1 VYSTUP_test_konzole_anim.txt
g++ -std=c++17 -O2 -w -I$INC -o $PRAC/konzole_nahled konzole_nahled.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
mkdir -p $PRAC/konzole
timeout 600 $PRAC/konzole_nahled $PRAC/konzole 600 800 2>&1 | cisty > VYSTUP_konzole_nahled.txt; tail -1 VYSTUP_konzole_nahled.txt

echo "== 8) shadery (vcetne noveho FS_KONZOLE) prisne podle GLSL ES 1.00"
python3 kontrola_shaderu.py > /dev/null; tail -1 VYSTUP_kontrola_shaderu.txt

echo "== 9) Java proti SKUTECNEMU Androidu a podpisy JNI (23 native metod)"
if [ -n "${ANDROID_JAR:-}" ] && [ -f "$ANDROID_JAR" ]; then
  rm -rf $PRAC/javac_out && mkdir -p $PRAC/javac_out
  { echo "B313 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    (cd $R && timeout 900 javac -proc:none --release 17 -cp "$ANDROID_JAR" -Xmaxerrs 1000 -nowarn -d $PRAC/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -v JAVA_TOOL)
    echo "trid prelozeno: $(find $PRAC/javac_out -name '*.class' | wc -l)"
    n=$(cd $R && javac -proc:none --release 17 -cp "$ANDROID_JAR" -nowarn -d $PRAC/javac_out app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -c "error:")
    echo "VYSLEDEK: $n chyb prekladu"; } > VYSTUP_javac_android.txt
  tail -1 VYSTUP_javac_android.txt
  rm -rf $PRAC/jnih && mkdir -p $PRAC/jnih/h $PRAC/jnih/out
  (cd $R && javac -proc:none --release 17 -cp "$ANDROID_JAR" -h $PRAC/jnih/h -d $PRAC/jnih/out app/src/main/java/eu/atarihelp/emu10/NativeMenuBridge.java 2>&1 | grep -v JAVA_TOOL)
  python3 kontrola_jni.py $PRAC/jnih/h/eu_atarihelp_emu10_NativeMenuBridge.h | tail -1
else
  echo "   (preskoceno - ANDROID_JAR neni nastaveny)"
fi
g++ -std=c++17 -O2 -fPIC -fvisibility=hidden -w -c $JNI -I../../app/src/main/cpp/menu -I../../app/src/main/cpp/intro -o $PRAC/nap_menu_gl.o ../../app/src/main/cpp/menu/nap_menu_gl.cpp || echo CHYBA_PREKLAD
echo "   JNI funkci viditelnych z knihovny (-fvisibility=hidden, JNIEXPORT): $(readelf -sW $PRAC/nap_menu_gl.o | grep ' GLOBAL DEFAULT .* Java_eu_atarihelp_emu10_NativeMenuBridge_' | wc -l) z 23" | tee -a VYSTUP_kontrola_jni.txt

echo "== 10) regrese: PS1 / Sega / demo na TV (kodeky, pasy, rozbocovac + TSan), cele intro, intro z Javy, menu (Mesa)"
bash regrese.sh "$PRAC" "$INC" "$JV" > /dev/null 2>&1; tail -1 VYSTUP_regrese.txt
