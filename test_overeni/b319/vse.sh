#!/bin/bash
# B319: PRENOS PS1 NA TV / PC - POSUN OBRAZU (KOPIE V PROHLIZECI), 2x ZVETSENY OBRAZ V PUVODNIM ROZLISENI,
# MYS Z PC JEDNIM SPOJENIM. Vsechny testy najednou (vystupy VYSTUP_* sem). Spoustet odkudkoli:
#   PRAC=... INC=... ANDROID_JAR=... PNG320=... PNG640=... bash test_overeni/b319/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): g++ / gcc, clang + ld.lld + llvm-objcopy (PS-EXE pro mipsel - regrese
# mysi B318), JAVA_HOME = JDK, hlavicky EGL / GLES2 / KHR v $INC, knihovny Mesa (libEGL, libGLESv2), zlib,
# Python 3 + Playwright + Chromium + numpy, Node.js (acorn - doinstaluje se do PRAC), android.jar API 34
# ($ANDROID_JAR), snimky 320x240 a 640x480 ($PNG320, $PNG640 - ../b305/JAK_SPUSTIT.txt krok 0), git.
set -u
PRAC=${PRAC:-/tmp/nap_b319}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
AJAR=${ANDROID_JAR:-}
PNG320=${PNG320:-$PRAC/png320}
PNG640=${PNG640:-$PRAC/png640}
R=$(cd "$(dirname "$0")/../.." && pwd)
Z=$R/test_overeni/b319
mkdir -p $PRAC
cd $Z
export EGL_PLATFORM=surfaceless
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
TVP="-I$R/app/src/main/cpp/tvproud -I$R/app/src/main/cpp/vendor/stb -I."

echo "== 1) kodek PS1 proudu (Hub + KodekJpeg z appky): posun obrazu, polovicni rozliseni, 3D hra, vzory, rozmery + Hub ve vlaknech + TSan"
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -pthread $TVP -o $PRAC/test_b319_kodek test_b319_kodek.cpp 2> $PRAC/preklad_kodek.txt || echo CHYBA_PREKLAD
$PRAC/test_b319_kodek | cisty > VYSTUP_test_b319_kodek.txt; tail -1 VYSTUP_test_b319_kodek.txt
g++ -std=c++17 -O2 -pthread $TVP -o $PRAC/test_b319_hub test_b319_hub.cpp || echo CHYBA_PREKLAD
$PRAC/test_b319_hub | cisty > VYSTUP_test_b319_hub.txt; tail -1 VYSTUP_test_b319_hub.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread $TVP -o $PRAC/test_b319_hub_tsan test_b319_hub.cpp || echo CHYBA_PREKLAD_TSAN
NAP_TSAN=1 TSAN_OPTIONS="halt_on_error=0" $PRAC/test_b319_hub_tsan > $PRAC/hub_tsan.txt 2>&1
{ grep -v "^WARNING\|^  \|^SUMMARY\|^==\|^$" $PRAC/hub_tsan.txt | cisty
  echo "hlaseni ThreadSanitizer: $(grep -c 'WARNING: ThreadSanitizer' $PRAC/hub_tsan.txt)"; } > VYSTUP_tsan_b319_hub.txt
tail -1 VYSTUP_tsan_b319_hub.txt

echo "== 2) stranka WEB TV v Chromiu: proud B319 (kopie + JPEG v polovicnim rozliseni) - obraz v canvasu = snimek hry; stara stranka B318"
g++ -std=c++17 -O2 -pthread $TVP -o $PRAC/gen_proud_b319 gen_proud_b319.cpp || echo CHYBA_PREKLAD
for v in 3 0; do mkdir -p $PRAC/proud_v$v && $PRAC/gen_proud_b319 $PRAC/proud_v$v $v > $PRAC/gen_v$v.txt; done
timeout 900 python3 test_b319_prohlizec.py $PRAC > $PRAC/prohlizec.txt 2>&1; tail -1 VYSTUP_test_b319_prohlizec.txt

echo "== 3) stranka WEB TV: mys pocitace (nejvys ~30 zprav / s, tlacitka hned) + vse z B318; skripty stranky (acorn)"
timeout 900 python3 test_b319_tv_mys.py > $PRAC/tv_mys.txt 2>&1; tail -1 VYSTUP_test_b319_tv_mys.txt
if [ ! -d $PRAC/node_modules/acorn ]; then (cd $PRAC && npm install --silent acorn@8.11.3 > /dev/null 2>&1); fi
rm -f $PRAC/tv_skript*.js
python3 -c "
import sys, re; sys.path.insert(0, '.')
import test_b319_tv_mys as T
for i, s in enumerate(re.findall(r'<script>([\s\S]*?)</script>', T.CZ)): open('$PRAC/tv_skript%d.js' % i, 'w').write(s)
"
(cd $PRAC && node -e "
const acorn = require('acorn'), fs = require('fs');
let ch = 0, n = 0; const out = [];
for (const f of fs.readdirSync('.').filter(f => /^tv_skript\d+\.js$/.test(f)).sort()) { n++;
  const s = fs.readFileSync(f, 'utf8'), v = s.indexOf('B318 MYS PC') >= 0 ? 5 : 2017;   // skript mysi je ES5
  try { acorn.parse(s, {ecmaVersion: v}); out.push('OK     ' + f + ' (ES' + v + ', ' + s.length + ' znaku)'); }
  catch (e) { ch++; out.push('CHYBA  ' + f + ': ' + e.message); } }
out.push('VYSLEDEK: ' + n + ' skriptu stranky WEB TV, ' + ch + ' CHYBA');
fs.writeFileSync('$Z/VYSTUP_kontrola_skriptu_tv.txt', out.join('\n') + '\n');")
tail -1 VYSTUP_kontrola_skriptu_tv.txt

echo "== 4) mys z PC jednim spojenim: SKUTECNY kod serveru z MainActivity (napTvWebHandleClient ...) v JVM + Chromium"
timeout 600 python3 test_b319_spojeni.py $PRAC/spojeni > $PRAC/spojeni.txt 2>&1; tail -1 VYSTUP_test_b319_spojeni.txt

echo "== 5) regrese (kodeky B305 / B313, PS1 B308, JVM, JNI, javac, mys B318 se skutecnym jadrem, stranka WEB TV: B302 / B303 / B306 / B313 / B314 / B315, menu)"
bash regrese.sh $PRAC $INC $JV "$AJAR" $PNG320 $PNG640 > /dev/null 2>&1; tail -1 VYSTUP_regrese.txt
