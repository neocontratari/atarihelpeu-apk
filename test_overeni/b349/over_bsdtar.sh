#!/bin/bash
# over_bsdtar.sh SLOZKA - NEZAVISLA kontrola testovacich archivu: co vyrobil vyrob_archivy.py, rozbali
# bsdtar (libarchive, cizi rozbalovac) do SLOZKA/bsdtar/<archiv>/ a porovna s puvodnimi daty (SLOZKA/obsah).
# Tim je overene, ze vlastni koder RAR 2.9 v testech pise opravdovy RAR (a nase Inflate / Rar29 ho
# rozbaluji spravne, ne jen "po svem").
# Zname rozdily bsdtar 3.7.x (ne chyby archivu):
#  - pevny (solid) RAR neumi ("RAR solid archive support unavailable") -> jen 1. soubor,
#  - po nove tabulce nenuluje "nizke bity" vzdalenosti (unrar, 7-Zip a libarchive od 2025 nuluji)
#    -> rar_nizke_po_tabulce.rar rozbali jinak (presne tenhle pripad testuje),
#  - zip_poskozeny_crc.zip ma schvalne poskozeny hra.xex, zip_heslo_a_hra.zip zaheslovany soubor (heslo
#    se nezadava, porovna se jen volna hra).
D=${1:?pouziti: over_bsdtar.sh SLOZKA_Z_vyrob_archivy.py}
bsdtar --version | head -1
for a in "$D"/*.rar "$D"/*.zip; do
  n=$(basename "$a")
  [ -d "$D/obsah/$n" ] || continue
  cil="$D/bsdtar/$n"
  mkdir -p "$cil"
  chyba=$(cd "$cil" && bsdtar -xf "$a" --passphrase neni < /dev/null 2>&1 | head -1 | cut -c1-100)
  ok=0; jinak=0; seznam=""
  while IFS= read -r f; do
    r=${f#"$D/obsah/$n/"}
    if cmp -s "$f" "$cil/$r"; then ok=$((ok + 1)); else jinak=$((jinak + 1)); seznam="$seznam $r"; fi
  done < <(find "$D/obsah/$n" -type f | sort)
  printf '%-28s %2d shodne s puvodnimi daty, %d jinak%s%s\n' "$n" "$ok" "$jinak" "${seznam:+ (}${seznam# }${seznam:+)}" "${chyba:+  | bsdtar: $chyba}"
done
