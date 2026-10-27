#!/bin/bash
# B317: MYS PLAYSTATION PRO PS1 (Dune 2000 ...) - vsechny testy najednou (vystupy VYSTUP_* sem)
# Spoustet odkudkoli: PRAC=... INC=... ANDROID_JAR=... bash test_overeni/b317/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): g++ / gcc, clang + ld.lld + llvm-objcopy (PS-EXE pro mipsel),
# JAVA_HOME = JDK, hlavicky EGL / GLES2 / KHR v $INC, knihovny Mesa (libEGL, libGLESv2), zlib,
# Python 3 + Playwright + Chromium + wordfreq, Node.js (acorn - doinstaluje se do PRAC), android.jar API 34, git.
set -u
PRAC=${PRAC:-/tmp/nap_b317}
INC=${INC:-$PRAC/inc}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
AJAR=${ANDROID_JAR:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
Z=$R/test_overeni/b317
mkdir -p $PRAC
cd $Z
export EGL_PLATFORM=surfaceless
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
PS1="-DANDROID -I../b305/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux"

echo "== 1) logika mysi v C++ (nap_ps1_mys.h): gesta, porty, tlacitka, D-pad, pamet pro hru, souhrn + ThreadSanitizer"
g++ -std=c++17 -O2 -Wall -Wextra -Wshadow -pthread -o $PRAC/test_b317_mys test_b317_mys.cpp || echo CHYBA_PREKLAD
mkdir -p $PRAC/mys && $PRAC/test_b317_mys $PRAC/mys | cisty > VYSTUP_test_b317_mys.txt; tail -1 VYSTUP_test_b317_mys.txt
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -DNAP_TSAN -o $PRAC/tsan_b317_mys test_b317_mys.cpp || echo CHYBA_PREKLAD_TSAN
TSAN_OPTIONS="halt_on_error=0" $PRAC/tsan_b317_mys $PRAC/mys > $PRAC/tsan_mys.txt 2>&1
{ cat $PRAC/tsan_mys.txt | grep -v "^WARNING\|^  \|^SUMMARY\|^==\|^$" | cisty
  echo "hlaseni ThreadSanitizer: $(grep -c 'WARNING: ThreadSanitizer' $PRAC/tsan_mys.txt)"; } > VYSTUP_tsan_b317_mys.txt
tail -1 VYSTUP_tsan_b317_mys.txt

echo "== 2) SKUTECNE jadro PS1 (PCSX-ReARMed z appky, BIOS HLE) + nap_ps1_native.cpp + hra mys_test.exe (BIOS InitPAD / StartPAD)"
bash stavba_jadra.sh $PRAC d7c85b9 > $PRAC/stavba_jadra.txt 2>&1; tail -2 $PRAC/stavba_jadra.txt | cisty
python3 udelej_exe.py $PRAC | cisty
g++ -std=c++17 -O1 -g -pthread $PS1 -o $PRAC/integrace_b317 integrace_b317.cpp $PRAC/jadro/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl 2> $PRAC/preklad_integrace.txt || echo CHYBA_PREKLAD
(cd $PRAC && timeout 600 ./integrace_b317 $PRAC $PRAC/mys_test.exe > integrace.txt 2>&1)
grep -E "^(B317|OK|CHYBA|VYSLEDEK)" $PRAC/integrace.txt | cisty > VYSTUP_integrace_b317.txt
cisty < $PRAC/UKAZKA_denik_mys.txt > UKAZKA_denik_mys.txt
#    srovnani: frontend/plugin.c z B316 (bez "posun se precte jednou") - hra cte port 2x v jednom snimku
OBJ=$(ls $PRAC/jadro/*.o | grep -v frontend_plugin.c.o)
g++ -std=c++17 -O1 -g -pthread $PS1 -o $PRAC/integrace_b316plugin integrace_b317.cpp $OBJ $PRAC/jadro_b316/frontend_plugin.c.o \
    -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl 2>/dev/null || echo CHYBA_PREKLAD_B316
(cd $PRAC && timeout 120 ./integrace_b316plugin $PRAC $PRAC/mys_test.exe --b316 2>/dev/null | grep "^B316" | cisty) >> VYSTUP_integrace_b317.txt
tail -1 VYSTUP_integrace_b317.txt; grep "VYSLEDEK" VYSTUP_integrace_b317.txt
#    ThreadSanitizer (dotyky z jineho vlakna za chodu vlakna emulace)
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread $PS1 -o $PRAC/integrace_b317_tsan integrace_b317.cpp $PRAC/jadro/*.o \
    -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl 2>/dev/null || echo CHYBA_PREKLAD_TSAN
(cd $PRAC && TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 detect_deadlocks=0" timeout 600 ./integrace_b317_tsan $PRAC $PRAC/mys_test.exe > tsan_integrace.txt 2>&1)
{ echo "B317 ThreadSanitizer - integrace se skutecnym jadrem (vlakno emulace + dotyky z jineho vlakna)"
  grep "^VYSLEDEK" $PRAC/tsan_integrace.txt
  echo "hlaseni ThreadSanitizer: $(grep -c 'WARNING: ThreadSanitizer' $PRAC/tsan_integrace.txt)"; } > VYSTUP_tsan_integrace_b317.txt
tail -1 VYSTUP_tsan_integrace_b317.txt

echo "== 3) stranka PS1 v Chromiu: skutecne dotyky -> logika mysi, tlacitko MYS, panel, cesky = B316 + mys, anglicky bez cestiny"
timeout 1200 python3 test_b317_stranka.py $PRAC > /dev/null 2>&1; tail -1 VYSTUP_test_b317_stranka.txt

echo "== 4) navod (napoveda CZ / EN): zmenilo se jen to o mysi, nazvy tlacitek v appce opravdu jsou"
python3 kontrola_navodu_b317.py > /dev/null; tail -1 VYSTUP_kontrola_navodu_b317.txt
(cd ../b312 && python3 kontrola_navodu.py > /dev/null 2>&1; cp VYSTUP_kontrola_navodu.txt $Z/VYSTUP_kontrola_navodu_cz.txt)
(cd ../b314 && python3 kontrola_navodu_en.py > /dev/null 2>&1; cp VYSTUP_kontrola_navodu_en.txt $Z/VYSTUP_kontrola_navodu_en.txt)
(cd ../b314 && timeout 900 python3 test_b314_jazyk_navod.py $PRAC/navod > $PRAC/navod_b314.txt 2>&1)
(cd $R && git checkout -- test_overeni/b312 test_overeni/b314 2>/dev/null)
nec=$(grep "^CHYBA  5) cesky navod" $PRAC/navod_b314.txt | sed -n "s/.*NECEKANE: //p")
jine=$(grep "^CHYBA" $PRAC/navod_b314.txt | grep -v "^CHYBA  5) cesky navod: texty jako v B313" | wc -l)
spr=$(python3 -c "
import ast, sys
s = sys.argv[1].strip()
v = ast.literal_eval(s) if s else []
ok = {'VYSUNOUT CARTRIDGE', '(Atari znovu s BASICem) a', 'VYBRAT JINÝ SOUBOR',             # B316
      'MYŠ PLAYSTATION (DUNE 2000 …)', 'Myš PlayStation (Dune 2000 …)', 'MYŠ', 'DRŽÍM LEVÉ TLAČÍTKO',   # B317
      'MYŠ V PORTU 1 (DUNE 2000)', 'MYŠ PORT 1 + OVLADAČ PORT 2', 'OVLADAČ PORT 1 + MYŠ PORT 2'}
print(1 if set(v) <= ok else 0)" "$nec")
{ echo "B317 - navod v Chromiu (test ../b314/test_b314_jazyk_navod.py na kodu B317)"
  echo "kontrola 5) 'cesky navod = B313 + zmeny B314' od B316 zamerne neplati (cartridge B316, mys B317) -"
  echo "co presne se zmenilo, hlida kontrola_navodu_b317.py; necekane texty v 5): $nec"
  cisty < $PRAC/navod_b314.txt
  echo "VYSLEDEK: jinych chyb nez 5): $jine, necekane texty jen z B316 / B317: $([ "$spr" = "1" ] && echo ANO || echo NE)"; } > VYSTUP_navod_b314_chromium.txt
tail -1 VYSTUP_navod_b314_chromium.txt; tail -1 VYSTUP_kontrola_navodu_cz.txt; tail -1 VYSTUP_kontrola_navodu_en.txt

echo "== 5) skripty v ES5 (stare WebView): slovnik en_ps1.js a skript stranky PS1"
if [ ! -d $PRAC/node_modules/acorn ]; then (cd $PRAC && npm install --silent acorn@8.11.3 > /dev/null 2>&1); fi
(cd $PRAC && node -e "
const acorn = require('acorn'), fs = require('fs');
const out = []; let ch = 0;
const html = fs.readFileSync('$R/app/src/main/assets/emu_ps1/index.html', 'utf8');
const re = /<script>([\s\S]*?)<\/script>/g; let m, i = 0;
while ((m = re.exec(html))) { i++; try { acorn.parse(m[1], {ecmaVersion: 5}); out.push('OK     ES5 emu_ps1/index.html skript ' + i + ' (' + m[1].length + ' znaku)'); }
  catch (e) { ch++; out.push('CHYBA  emu_ps1/index.html skript ' + i + ': ' + e.message); } }
for (const f of ['jazyk/en_ps1.js', 'spolecne/nap_jazyk.js', 'jazyk/en_konzole.js']) {
  try { acorn.parse(fs.readFileSync('$R/app/src/main/assets/' + f, 'utf8'), {ecmaVersion: 5}); out.push('OK     ES5 ' + f); }
  catch (e) { ch++; out.push('CHYBA  ' + f + ': ' + e.message); } }
out.push('VYSLEDEK: ' + out.length + ' souboru / skriptu, ' + ch + ' CHYBA');
fs.writeFileSync('$Z/VYSTUP_kontrola_es5.txt', out.join('\n') + '\n');")
tail -1 VYSTUP_kontrola_es5.txt

echo "== 6) regrese PS1 (testy B308 na kodu B317, JVM, JNI, javac, TV B305 / B306)"
bash regrese.sh $PRAC $INC $JV "$AJAR" > /dev/null 2>&1; tail -1 VYSTUP_regrese.txt
