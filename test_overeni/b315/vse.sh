#!/bin/bash
# B315: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b315)
#   ZVUK NA TV / PC HRAL JEN V PRVNI HRE (Rene, log B314, uz od B313).
#   Zmena jen ve strance WEB TV (MainActivity.napTvWebWriteHtml): proud z C++ ma vlastni zesilovac,
#   navazani stare cesty (/audio.raw) nenecha zesilovac na 0 ani pri zpozdeni 0, sipky v menu
#   zpozdeni nemeni, mereni vystupu do logu appky.
# Spoustet odkudkoli: PRAC=... INC=... PNG320=... ANDROID_JAR=... bash test_overeni/b315/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): Python 3 + numpy, Chromium + Playwright pro Python, git,
# g++ / gcc, JAVA_HOME = JDK, hlavicky EGL / GLES2 / KHR v $INC (jako ../b311), knihovny Mesa,
# snimky 320x240 ($PNG320, ../b305/JAK_SPUSTIT.txt krok 0), android.jar API 34 ($ANDROID_JAR).
set -u
PRAC=${PRAC:-/tmp/nap_b315}
INC=${INC:-$PRAC/inc}
PNG320=${PNG320:-$PRAC/png320}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b315

echo "== 1) zvuk na TV / PC po prvni hre: stranka z MainActivity.java v Chromiu, postup z logu Reneho, B314 proti B315"
timeout 1800 python3 test_b315_zvuk_tv.py > $PRAC/b315_zvuk_log.txt 2>&1; tail -1 VYSTUP_test_b315_zvuk_tv.txt

echo "== 2) skripty stranky WEB TV bez chyby syntaxe (acorn, ES2017 jako dosud - async pollAudio)"
if [ ! -d $PRAC/node_modules/acorn ]; then (cd $PRAC && npm install --silent acorn@8.11.3 > /dev/null 2>&1); fi
python3 -c "
import sys, re; sys.path.insert(0, '.')
import test_b315_zvuk_tv as T
h = T.stranka_z_javy(open(T.KOREN + '/' + T.JAVA_CESTA, encoding='utf-8').read())
for i, s in enumerate(re.findall(r'<script>([\s\S]*?)</script>', h)): open('$PRAC/tv_skript%d.js' % i, 'w').write(s)
"
NODE_PATH=$PRAC/node_modules node -e "
const acorn = require('acorn'), fs = require('fs');
let ch = 0, n = 0; const out = [];
for (const f of fs.readdirSync('$PRAC').filter(f => /^tv_skript\d+\.js$/.test(f))) { n++;
  try { acorn.parse(fs.readFileSync('$PRAC/' + f, 'utf8'), {ecmaVersion: 2017}); out.push('OK     ' + f); }
  catch (e) { ch++; out.push('CHYBA  ' + f + ': ' + e.message); } }
out.push('VYSLEDEK: ' + n + ' skriptu stranky WEB TV, ' + ch + ' CHYBA');
fs.writeFileSync('VYSTUP_kontrola_skriptu_tv.txt', out.join('\n') + '\n');"
tail -1 VYSTUP_kontrola_skriptu_tv.txt
echo "   stranka B315 = stranka B314 (git 8629955) + JEN zmeny zvuku (difflib po prikazech):"
python3 kontrola_stranky.py > /dev/null; echo "   $(tail -1 VYSTUP_kontrola_stranky.txt)"

echo "== 3) Java proti SKUTECNEMU Androidu (android.jar API 34)"
if [ -n "${ANDROID_JAR:-}" ] && [ -f "$ANDROID_JAR" ]; then
  rm -rf $PRAC/javac_out && mkdir -p $PRAC/javac_out
  { echo "B315 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    n=$(cd $R && timeout 900 $JV/bin/javac -proc:none --release 17 -cp "$ANDROID_JAR" -Xmaxerrs 1000 -nowarn -d $PRAC/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -v JAVA_TOOL | tee $PRAC/javac.txt | grep -c "error:")
    cat $PRAC/javac.txt
    echo "trid prelozeno: $(find $PRAC/javac_out -name '*.class' | wc -l)"
    echo "VYSLEDEK: $n chyb prekladu"; } > VYSTUP_javac_android.txt
  tail -1 VYSTUP_javac_android.txt
else
  echo "   (preskoceno - ANDROID_JAR neni nastaveny)"
fi

echo "== 4) regrese: testy stranky WEB TV z B302, B303, B306, B313, B314 se strankou B315"
bash regrese.sh "$PRAC" "$INC" "$JV" "$PNG320" > /dev/null 2>&1; tail -1 VYSTUP_regrese.txt
