#!/bin/bash
# B322 - regrese: starsi testy, kterych se B322 dotyka (zamky Segy / PS1, menu, navod CZ / EN, odkazy,
# mosty Javy, stranky aktivace a N&P HRY), + javac cele appky proti android.jar.
# Pouziti (z test_overeni/b322): bash regrese.sh <android.jar> [slozka B321 = git worktree aab4eed]
#  - se slozkou B321 se tytez testy pusti i na B321 a porovna se, ktere kontroly padaji uz tam
#    (= stare, ne z B322). Vystupy starsich testu v gitu se na konci vrati (git checkout).
AJAR="$1"; B321="$2"
R="$(cd ../.. && pwd)"; Z="$(pwd)"; OUT="$Z/VYSTUP_regrese.txt"; P="$(mktemp -d)"
echo "B322 - regrese (starsi testy na kodu B322; s B321 = srovnani se stejnymi testy na commitu aab4eed)" > "$OUT"
echo "" >> "$OUT"
TESTY=("b311/kontrola_odkazu.py" "b311/kontrola_navodu.py" "b314/kontrola_navodu_en.py" "b312/kontrola_zamku.py" "b314/kontrola_zamku_texty.py"
       "b312/kontrola_mostu.py" "b311/kontrola_mostu.py" "b320/test_b320_stranky.py" "b314/test_b314_jazyk_navod.py"
       "b314/test_b314_jazyk_stranky.py sega ps1" "b314/test_b314_jazyk_menu.py" "b311/test_b311_menu.py" "b321/test_b321_stranky.py")
chyby() { grep -E "^CHYBA" "$1" | sed -E 's/ - ROZDIL.*| - NECEKANE.*|\(\+[0-9]+ -[0-9]+:.*|   -> .*//' | cut -c1-140 | sort -u; }
nove=0
for t in "${TESTY[@]}"; do
  set -- $t; j=$(basename "$1" .py)
  (cd "$R/test_overeni/$(dirname "$1")" && timeout 900 python3 "$(basename "$1")" "${@:2}" > "$P/$j.b322.txt" 2>&1)
  v=$(grep -E "VYSLEDEK|^Chyb|CHYB:" "$P/$j.b322.txt" | tail -1)
  echo "== $t: $v" >> "$OUT"
  if [ -n "$B321" ] && [ -d "$B321/test_overeni" ]; then
    (cd "$B321/test_overeni/$(dirname "$1")" && timeout 900 python3 "$(basename "$1")" "${@:2}" > "$P/$j.b321.txt" 2>&1)
    echo "   B321: $(grep -E "VYSLEDEK|^Chyb|CHYB:" "$P/$j.b321.txt" | tail -1)" >> "$OUT"
    comm -23 <(chyby "$P/$j.b322.txt") <(chyby "$P/$j.b321.txt") > "$P/$j.nove"
    if [ -s "$P/$j.nove" ]; then nove=$((nove+1)); sed 's/^/   NOVA CHYBA (na B321 nebyla): /' "$P/$j.nove" >> "$OUT"
    else chyby "$P/$j.b322.txt" | sed 's/^/   stejna jako na B321: /' >> "$OUT"; fi
  else
    chyby "$P/$j.b322.txt" | sed 's/^/   /' >> "$OUT"
  fi
done
if [ -n "$AJAR" ] && [ -f "$AJAR" ]; then
  mkdir -p "$P/javac"
  (cd "$R" && timeout 900 javac -proc:none --release 17 -cp "$AJAR" -Xmaxerrs 1000 -nowarn -d "$P/javac" \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java > "$P/javac.txt" 2>&1)
  echo "== javac --release 17 -cp android.jar (API 34), cela appka: $(grep -c 'error:' "$P/javac.txt") chyb, trid $(find "$P/javac" -name '*.class' | wc -l)" >> "$OUT"
fi
(cd "$R" && git checkout -- test_overeni/b311 test_overeni/b312 test_overeni/b314 test_overeni/b320 test_overeni/b321 2>/dev/null; rm -f test_overeni/b314/VYSTUP_test_b314_jazyk_stranky_ps1_sega.txt)
echo "" >> "$OUT"
echo "Zamerne zmeny B322, ktere starsi testy (porovnani s texty B312 - B321) hlasi jako rozdil: zamky Segy a PS1" >> "$OUT"
echo "(1500 radku / 120 hodin misto 50 radku / 30 minut, '- nebo odemkni hned'), tlacitko zamcene karty ODEMKNOUT" >> "$OUT"
echo "misto ZAMCENO, navod CZ / EN (kapitola ODEMYKANI s platbou, zaloha postupu na serveru)." >> "$OUT"
echo "b311/test_b311_menu.py A6 a b314/test_b314_jazyk_menu.py: tlacitko u zamcene karty v HTML zaloze menu uz neukazuje" >> "$OUT"
echo "hlasku, ale otevre stranku hry/odemknout.html (zamerne) - test b314 po tom kroku dal nepokracuje (je na jine strance)." >> "$OUT"
echo "VYSLEDEK: $nove testu s NOVOU chybou proti B321" >> "$OUT"
rm -rf "$P"
cat "$OUT"
