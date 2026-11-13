#!/bin/bash
# B321 - regrese: starsi testy, kterych se B321 dotyka (stranky Sega / PS1 / OPTIONS, menu karty.txt,
# navod CZ / EN, odkazy), + javac cele appky proti android.jar.
# Pouziti (z test_overeni/b321): bash regrese.sh <android.jar> [slozka B320 = git worktree 54df992]
#  - se slozkou B320 se tytez testy pusti i na B320 a porovna se, ktere kontroly padaji uz tam
#    (= stare, ne z B321). Vystupy starsich testu v gitu se na konci vrati (git checkout).
AJAR="$1"; B320="$2"
R="$(cd ../.. && pwd)"; Z="$(pwd)"; OUT="$Z/VYSTUP_regrese.txt"; P="$(mktemp -d)"
echo "B321 - regrese (starsi testy na kodu B321; s B320 = srovnani se stejnymi testy na commitu 54df992)" > "$OUT"
echo "" >> "$OUT"
TESTY=("b311/kontrola_odkazu.py" "b311/kontrola_navodu.py" "b314/kontrola_navodu_en.py" "b320/test_b320_stranky.py"
       "b314/test_b314_jazyk_navod.py" "b314/test_b314_jazyk_stranky.py sega ps1" "b314/test_b314_jazyk_menu.py" "b311/test_b311_menu.py")
chyby() { grep -E "^CHYBA" "$1" | sed -E 's/ - ROZDIL.*| - NECEKANE.*|\(\+[0-9]+ -[0-9]+:.*//' | cut -c1-140 | sort -u; }
nove=0
for t in "${TESTY[@]}"; do
  set -- $t; j=$(basename "$1" .py)
  (cd "$R/test_overeni/$(dirname "$1")" && timeout 900 python3 "$(basename "$1")" "${@:2}" > "$P/$j.b321.txt" 2>&1)
  v=$(grep -E "VYSLEDEK" "$P/$j.b321.txt" | tail -1)
  echo "== $t: $v" >> "$OUT"
  if [ -n "$B320" ] && [ -d "$B320/test_overeni" ]; then
    (cd "$B320/test_overeni/$(dirname "$1")" && timeout 900 python3 "$(basename "$1")" "${@:2}" > "$P/$j.b320.txt" 2>&1)
    echo "   B320: $(grep -E "VYSLEDEK" "$P/$j.b320.txt" | tail -1)" >> "$OUT"
    comm -23 <(chyby "$P/$j.b321.txt") <(chyby "$P/$j.b320.txt") > "$P/$j.nove"
    if [ -s "$P/$j.nove" ]; then nove=$((nove+1)); sed 's/^/   NOVA CHYBA (na B320 nebyla): /' "$P/$j.nove" >> "$OUT"
    else chyby "$P/$j.b321.txt" | sed 's/^/   stejna jako na B320: /' >> "$OUT"; fi
  else
    chyby "$P/$j.b321.txt" | sed 's/^/   /' >> "$OUT"
  fi
done
if [ -n "$AJAR" ] && [ -f "$AJAR" ]; then
  mkdir -p "$P/javac"
  (cd "$R" && timeout 900 javac -proc:none --release 17 -cp "$AJAR" -Xmaxerrs 1000 -nowarn -d "$P/javac" \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java > "$P/javac.txt" 2>&1)
  echo "== javac --release 17 -cp android.jar (API 34), cela appka: $(grep -c 'error:' "$P/javac.txt") chyb, trid $(find "$P/javac" -name '*.class' | wc -l)" >> "$OUT"
fi
(cd "$R" && git checkout -- test_overeni/b311 test_overeni/b314 test_overeni/b320 2>/dev/null; rm -f test_overeni/b314/VYSTUP_test_b314_jazyk_stranky_ps1_sega.txt)
echo "" >> "$OUT"
echo "Zamerne zmeny B321, ktere starsi testy (porovnani textu s B313) hlasi jako rozdil: karta ATARI 130XE v menu" >> "$OUT"
echo "('hry ze serveru N&P'), navod CZ / EN (aktivace, N&P HRY, web v prohlizeci, aktualizace ze serveru)." >> "$OUT"
echo "VYSLEDEK: $nove testu s NOVOU chybou proti B320" >> "$OUT"
rm -rf "$P"
cat "$OUT"
