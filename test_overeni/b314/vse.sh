#!/bin/bash
# B314: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b314)
#   JAZYK CZ / EN: OPTIONS -> JAZYK · LANGUAGE (CESTINA / ENGLISH), poprve podle jazyka telefonu.
#   Menu (C++ i zalozni HTML), OPTIONS, HELP (napoveda/en.html), Atari 130XE, Sega, PS1, MP3,
#   DZUBOX, hlasky Javy, intro, stranka WEB TV na TV / PC.
# Spoustet odkudkoli: PRAC=... INC=... ANDROID_JAR=... [GLSLANG=...] bash test_overeni/b314/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): g++, JAVA_HOME = JDK, hlavicky EGL / GLES2 / KHR +
# stb_image_write.h v $INC (jako ../b311), knihovny Mesa (libEGL, libGLESv2), zlib, Python 3 +
# numpy + Pillow + wordfreq, Chromium + Playwright pro Python, Node.js (acorn - doinstaluje se do PRAC),
# android.jar API 34 ($ANDROID_JAR), ffmpeg / ffprobe (regrese B312).
set -u
PRAC=${PRAC:-/tmp/nap_b314}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b314
export EGL_PLATFORM=surfaceless
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
JNI="-I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud"
A=../../app/src/main/assets

echo "== 1) hlavni menu v Chromiu: OPTIONS (JAZYK · LANGUAGE tam i zpet za chodu), zalozni karusel, zamky"
timeout 900 python3 test_b314_jazyk_menu.py > /dev/null 2>&1; tail -1 VYSTUP_test_b314_jazyk_menu.txt

echo "== 2) stranky cesky = B313, anglicky bez cestiny, bez nove chyby JS: Atari 130XE, Sega, PS1, MP3, DZUBOX"
timeout 2400 python3 test_b314_jazyk_stranky.py > /dev/null 2>&1; tail -1 VYSTUP_test_b314_jazyk_stranky.txt

echo "== 3) navod: anglicky napoveda/en.html, presmerovani podle jazyka appky, hledani; nazvy tlacitek"
timeout 900 python3 test_b314_jazyk_navod.py $PRAC/navod > /dev/null 2>&1; tail -1 VYSTUP_test_b314_jazyk_navod.txt
python3 kontrola_navodu_en.py > /dev/null; tail -1 VYSTUP_kontrola_navodu_en.txt
(cd ../b311 && cp VYSTUP_kontrola_navodu.txt $PRAC/navod_b311_puvodni.txt && python3 kontrola_navodu.py > /dev/null 2>&1; \
  cp VYSTUP_kontrola_navodu.txt ../b314/VYSTUP_kontrola_navodu_cz.txt; cp $PRAC/navod_b311_puvodni.txt VYSTUP_kontrola_navodu.txt)
echo "   cesky navod (B311 kontrola): $(tail -1 VYSTUP_kontrola_navodu_cz.txt)"

echo "== 4) stranka WEB TV z MainActivity.java: cesky = B313, anglicky (jak ji sklada Java) bez cestiny"
timeout 1500 python3 test_b314_jazyk_tv.py > /dev/null 2>&1; tail -1 VYSTUP_test_b314_jazyk_tv.txt

echo "== 5) C++ bez GPU: karty.txt s anglickymi sloupci, pouzijJazyk, Atari tx / stitky / chyby kazety"
g++ -std=c++17 -O2 -o $PRAC/test_jazyk_cpp test_jazyk_cpp.cpp || echo CHYBA_PREKLAD
$PRAC/test_jazyk_cpp > VYSTUP_test_jazyk_cpp.txt 2>&1; tail -1 VYSTUP_test_jazyk_cpp.txt

echo "== 6) menu anglicky pres Mesa (presne C++ appky): snimky + obrazky karet pro HTML = assets"
g++ -std=c++17 -O2 -w -I$INC -o $PRAC/menu_snimky_b314 menu_snimky_b314.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
rm -rf $PRAC/menu_en $PRAC/karty_cz $PRAC/karty_en && mkdir -p $PRAC/menu_en $PRAC/karty_cz $PRAC/karty_en
{ timeout 900 $PRAC/menu_snimky_b314 $PRAC/menu_en 1080 2340 --en 0:3 3:3 6:3
  timeout 600 $PRAC/menu_snimky_b314 $PRAC/menu_en 1080 2340 --en --zamky 1:3 2:3
  timeout 600 $PRAC/menu_snimky_b314 $PRAC/menu_en 2340 1080 --en 6:3
  timeout 600 $PRAC/menu_snimky_b314 $PRAC/menu_en 1280 720 --en --tv 0:3
  timeout 600 $PRAC/menu_snimky_b314 $PRAC/karty_cz 600 800 --karty
  timeout 600 $PRAC/menu_snimky_b314 $PRAC/karty_en 600 800 --en --karty; } 2>&1 | grep -v "^DEMO" | cisty > VYSTUP_menu_snimky_en.txt
python3 - "$PRAC" "$A" >> VYSTUP_menu_snimky_en.txt << 'PYEOF'
import sys, numpy as np
from PIL import Image
prac, a = sys.argv[1], sys.argv[2]
def rozdil(x, y):
    p = np.asarray(Image.open(x).convert('RGB')).astype(int); q = np.asarray(Image.open(y).convert('RGB')).astype(int)
    return abs(p - q).mean() if p.shape == q.shape else 999.0
ch = 0
for n in ('atari', 'sega', 'ps1', 'mp3', 'dzubox', 'web', 'help'):
    d = rozdil('%s/karty_cz/karta_%s.jpg' % (prac, n), '%s/menu/karta_%s.jpg' % (a, n))
    ch += d > 0.5
    print('%s karta_%s.jpg z C++ = assets (rozdil %.2f)' % ('OK   ' if d <= 0.5 else 'CHYBA', n, d))
d = rozdil('%s/karty_en/karta_help_en.jpg' % prac, '%s/menu/karta_help_en.jpg' % a)
ch += d > 0.5
print('%s karta_help_en.jpg (kniha GUIDE / CONTROLS) z C++ = assets (rozdil %.2f)' % ('OK   ' if d <= 0.5 else 'CHYBA', d))
for n in ('atari', 'sega', 'ps1', 'mp3', 'dzubox', 'web'):
    d = rozdil('%s/karty_cz/karta_%s.jpg' % (prac, n), '%s/karty_en/karta_%s_en.jpg' % (prac, n))
    ch += d > 0.5
    print('%s karta_%s anglicky = cesky (obrazek bez textu, rozdil %.2f)' % ('OK   ' if d <= 0.5 else 'CHYBA', n, d))
print('VYSLEDEK: obrazky karet %d CHYBA' % ch)
PYEOF
echo "   $(grep -c '^snimek' VYSTUP_menu_snimky_en.txt) snimku menu anglicky, CHYB GL nenulovych: $(grep 'CHYB GL celkem' VYSTUP_menu_snimky_en.txt | grep -vc 'celkem: 0'); $(tail -1 VYSTUP_menu_snimky_en.txt)"

echo "== 7) SKUTECNY nap_menu_gl.cpp pres JNI (jako z Javy) + menuJazyk tam a zpet (+ ThreadSanitizer)"
g++ -std=c++17 -O2 -pthread -w $JNI -o $PRAC/test_menu_host_b314 test_menu_host_b314.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
PRAC=$PRAC timeout 900 $PRAC/test_menu_host_b314 2>&1 | grep -v "^karta \|^DEMO\|^MENU" | cisty > VYSTUP_test_menu_host_b314.txt; tail -1 VYSTUP_test_menu_host_b314.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -w $JNI -o $PRAC/test_menu_host_b314_tsan test_menu_host_b314.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 || echo CHYBA_PREKLAD
TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0" timeout 1800 $PRAC/test_menu_host_b314_tsan > $PRAC/tsan_menu_b314.txt 2>&1
python3 tsan_rozbor.py $PRAC/tsan_menu_b314.txt > /dev/null; tail -1 VYSTUP_tsan_menu_host_b314.txt

echo "== 8) uvodni demo cesky / anglicky (Mesa, presne C++ appky): titulky a zaverecne titulky, jinak stejne"
g++ -std=c++17 -O2 -w -I$INC -o $PRAC/intro_jazyk intro_jazyk.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz || echo CHYBA_PREKLAD
mkdir -p $PRAC/intro_en
timeout 1200 $PRAC/intro_jazyk $PRAC/intro_en 854 480 2>&1 | grep -v "^DEMO\|^NAP_DEMO" | cisty > VYSTUP_intro_jazyk.txt; tail -1 VYSTUP_intro_jazyk.txt

echo "== 9) pristroj Atari 130XE cesky / anglicky (C++ nap_atari_device.h, jako po devCtl jazyk:en)"
g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o $PRAC/atari_jazyk atari_jazyk.cpp || echo CHYBA_PREKLAD
mkdir -p $PRAC/atari_en
$PRAC/atari_jazyk $PRAC/atari_en 2>&1 | cisty > VYSTUP_atari_jazyk.txt; tail -1 VYSTUP_atari_jazyk.txt

echo "== 10) JVM: NapPravidla cesky / anglicky (zamky, karta HELP); vety zamku Java (C++ menu) = HTML menu"
rm -rf $PRAC/jvm && mkdir -p $PRAC/jvm
$JV/bin/javac --release 17 -d $PRAC/jvm $R/app/src/main/java/eu/atarihelp/emu10/NapPravidla.java TestB314Java.java 2>&1 | grep -v JAVA_TOOL
$JV/bin/java -Dstdout.encoding=UTF-8 -cp $PRAC/jvm eu.atarihelp.emu10.TestB314Java 2>&1 | grep -v JAVA_TOOL > VYSTUP_test_b314_java.txt; tail -1 VYSTUP_test_b314_java.txt
JAVA_HOME=$JV python3 kontrola_zamku_texty.py $PRAC/jvm > /dev/null; tail -1 VYSTUP_kontrola_zamku_texty.txt

echo "== 11) skripty jazyka v ES5 (stare WebView), slovniky bez chyby syntaxe"
if [ ! -d $PRAC/node_modules/acorn ]; then (cd $PRAC && npm install --silent acorn@8.11.3 > /dev/null 2>&1); fi
NODE_PATH=$PRAC/node_modules node -e "
const acorn = require('acorn'), fs = require('fs'), p = require('path');
const sou = ['$A/spolecne/nap_jazyk.js'].concat(fs.readdirSync('$A/jazyk').map(f => '$A/jazyk/' + f));
let ch = 0; const out = [];
for (const f of sou) { try { acorn.parse(fs.readFileSync(f, 'utf8'), {ecmaVersion: 5}); out.push('OK     ES5 ' + p.basename(f)); }
  catch (e) { ch++; out.push('CHYBA  ' + p.basename(f) + ': ' + e.message); } }
out.push('VYSLEDEK: ' + sou.length + ' souboru, ' + ch + ' CHYBA');
fs.writeFileSync('VYSTUP_kontrola_es5.txt', out.join('\n') + '\n');" 2>&1
tail -1 VYSTUP_kontrola_es5.txt

echo "== 12) Java proti SKUTECNEMU Androidu a podpisy JNI (menu 24 native metod, intro 17) + export z knihoven"
if [ -n "${ANDROID_JAR:-}" ] && [ -f "$ANDROID_JAR" ]; then
  rm -rf $PRAC/javac_out && mkdir -p $PRAC/javac_out
  { echo "B314 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    (cd $R && timeout 900 $JV/bin/javac -proc:none --release 17 -cp "$ANDROID_JAR" -Xmaxerrs 1000 -nowarn -d $PRAC/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -v JAVA_TOOL)
    echo "trid prelozeno: $(find $PRAC/javac_out -name '*.class' | wc -l)"
    n=$(cd $R && $JV/bin/javac -proc:none --release 17 -cp "$ANDROID_JAR" -nowarn -d $PRAC/javac_out app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -c "error:")
    echo "VYSLEDEK: $n chyb prekladu"; } > VYSTUP_javac_android.txt
  tail -1 VYSTUP_javac_android.txt
  rm -rf $PRAC/jnih && mkdir -p $PRAC/jnih/h $PRAC/jnih/out
  (cd $R && $JV/bin/javac -proc:none --release 17 -cp "$ANDROID_JAR" -h $PRAC/jnih/h -d $PRAC/jnih/out \
    app/src/main/java/eu/atarihelp/emu10/NativeMenuBridge.java app/src/main/java/eu/atarihelp/emu10/NativeIntroBridge.java 2>&1 | grep -v JAVA_TOOL)
  python3 kontrola_jni.py $PRAC/jnih/h | tail -1
else
  echo "   (preskoceno - ANDROID_JAR neni nastaveny)"
fi
g++ -std=c++17 -O2 -fPIC -fvisibility=hidden -w -c $JNI -I../../app/src/main/cpp/menu -I../../app/src/main/cpp/intro -o $PRAC/nap_menu_gl.o ../../app/src/main/cpp/menu/nap_menu_gl.cpp || echo CHYBA_PREKLAD
g++ -std=c++17 -O2 -fPIC -fvisibility=hidden -w -c -I../b306/stub $JNI -I../../app/src/main/cpp/intro -o $PRAC/nap_intro_gl.o ../../app/src/main/cpp/intro/nap_intro_gl.cpp || echo CHYBA_PREKLAD
{ echo "       z knihoven videt (-fvisibility=hidden, JNIEXPORT): menu $(readelf -sW $PRAC/nap_menu_gl.o | grep ' GLOBAL DEFAULT .* Java_eu_atarihelp_emu10_NativeMenuBridge_' | wc -l) z 24 (menuJazyk $(readelf -sW $PRAC/nap_menu_gl.o | grep -c 'GLOBAL DEFAULT .*NativeMenuBridge_menuJazyk')), intro $(readelf -sW $PRAC/nap_intro_gl.o | grep ' GLOBAL DEFAULT .* Java_eu_atarihelp_emu10_NativeIntroBridge_' | wc -l) z 17 (introJazyk $(readelf -sW $PRAC/nap_intro_gl.o | grep -c 'GLOBAL DEFAULT .*NativeIntroBridge_introJazyk'))"; } | tee -a VYSTUP_kontrola_jni.txt

echo "== 13) regrese: testy B313 a B312 na kodu B314 (vysledky sem, puvodni VYSTUP_* starsich verzi zpet z gitu)"
bash regrese.sh "$PRAC" "$INC" "$JV" > /dev/null 2>&1; tail -1 VYSTUP_regrese.txt
