#!/bin/bash
# B348 - regrese jadra Atari: stejne testy ve STAREM jadru (B347) a NOVEM (B348), porovnani vystupu.
# Pouziti (z test_overeni/b348): bash regrese_jadra.sh <slozka stareho repozitare> <pracovni slozka>
#   stary repozitar (jen zdroje jadra a testu): mkdir /tmp/b347 && git archive 7f0b755 app/src/main/cpp/atari \
#       test_overeni/b291 test_overeni/b292 test_overeni/b316 test_overeni/b323 | tar -x -C /tmp/b347
# Ocekavane rozdily B348: samotest cpuHash (LXA), jinak MUSI byt vystupy stejne (hry z test_assets nepouzivaji
# vlastni SIO ani LXA; bootuji pres OS = SIO patch jako drive).
STARY="$1"; P="$2"; R="$(cd ../.. && pwd)"; OUT="$(pwd)/VYSTUP_regrese_jadra.txt"
mkdir -p "$P/stare" "$P/nove"
preloz() {  # preloz <jadro> <cil> <zdroj>
  g++ -std=c++17 -O2 -w -I"$1/app/src/main/cpp/atari" -o "$2" "$3" -lz 2>&1 | head -5
}
for t in test_core:b292/test_core.cpp test_b291_runtime:b291/test_b291_runtime.cpp test_b316_core:b316/test_b316_core.cpp regrese_her:b323/regrese_her.cpp memhash:b323/memhash.cpp; do
  jm="${t%%:*}"; zdroj="$R/test_overeni/${t#*:}"
  preloz "$STARY" "$P/stare/$jm" "$zdroj"; preloz "$R" "$P/nove/$jm" "$zdroj"
done
echo "B348 - regrese jadra Atari: stare jadro (B347) vs nove (B348) - stejne testy, porovnani vystupu" > "$OUT"
echo "" >> "$OUT"
beh() {  # beh <jmeno> <prikaz...> (v adresari testu b292, aby nasel soubory)
  local jm="$1"; shift
  for k in stare nove; do
    (cd "$R/test_overeni/b292" && KLAVESY="$KLAVESY" JOY="$JOY" timeout 1200 "$P/$k/$@" > "$P/$k/$jm.txt" 2>&1)
  done
  # casy a rychlosti (ms, s, x) se lisi beh od behu - pryc; cesty ke slozkam taky
  for k in stare nove; do sed -E 's/[0-9]+(\.[0-9]+)? ?(ms|us|s|x|MIPS|krat)\b/#\2/g; s#'"$P"'/(stare|nove)#P#g' "$P/$k/$jm.txt" > "$P/$k/$jm.cist"; done
  if cmp -s "$P/stare/$jm.cist" "$P/nove/$jm.cist"; then echo "STEJNE  $jm ($(wc -l < "$P/nove/$jm.txt") radku)" >> "$OUT"
  else echo "ROZDIL  $jm:" >> "$OUT"; diff "$P/stare/$jm.cist" "$P/nove/$jm.cist" | head -12 | sed 's/^/        /' >> "$OUT"; fi
}
KLAVESY=""; JOY=""
beh boot test_core boot
beh xe130 test_core xe130
beh kazeta test_core kazeta
beh bootkazeta test_core bootkazeta synt
beh pokey test_core pokey-casovac
beh txt_basic test_core txt-program zkouska basic
beh txt_tbxl test_core txt-program zkouska tbxl
beh tbxl_disketa test_core tbxl-disketa
beh vlozeni test_core vlozeni
beh pc_klav test_core pc-klavesnice
beh tv_proud test_core tv-proud
KLAVESY="400:33,1600:1000,1800:1001" JOY="1900-2600:4" beh popeye test_core xex "../../test_assets/Popeye (VBXE, PAL Version)(2).xex" 2600 /dev/null 1500,2600
KLAVESY="400:33,700:12,900:12,1100:12,1570:33" JOY="1500-1560:1" beh wolf3d test_core atr "../../test_assets/wolf3d(1).atr" 1700 /dev/null 300,1400,1560,1700
KLAVESY=""; JOY=""
beh b291_runtime test_b291_runtime
beh b316_core test_b316_core
beh memhash memhash
# hry z test_assets: stejny scenar vstupu, otisky obrazu kazdych 100 snimku (b323/regrese_her.cpp)
for h in "$R"/test_assets/*; do
  jm="hra_$(basename "$h" | tr ' ()' '___')"
  beh "$jm" regrese_her "$h" 2000
done
echo "" >> "$OUT"
echo "VYSLEDEK: $(grep -c '^STEJNE' "$OUT") stejne, $(grep -c '^ROZDIL' "$OUT") s rozdilem" >> "$OUT"
cat "$OUT"
