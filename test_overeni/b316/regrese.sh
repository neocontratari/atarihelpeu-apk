#!/bin/bash
# B316: REGRESE - jadro Atari 130XE BEZ cartridge se musi chovat PRESNE jako v B315.
#   1) test_core (../b292) prelozeny dvakrat: s jadrem z B315 (git 0fd67c7) a s jadrem B316;
#      vsechny rezimy (BASIC, 130XE, POKEY, kazeta, TXT, Turbo-BASIC + disketa, klavesnice PC,
#      proud pro TV, Ctrl+V, Acid800, hry XEX a ATR vc. VBXE) - vystupy i snimky obrazu
#      (kody barev GTIA .idx a RGB .ppm) musi byt SHODNE (jen casy behu na PC se lisi)
#   2) test_b291_runtime (psani, CSAVE) B315 vs B316 - shodny vystup
#   3) testy skutecneho nap_atari_native.cpp s vlakny: ../b291 a ../b292 (B296-B303) - 0 chyb
#   4) navod v appce (napoveda): kontrola nazvu tlacitek cesky (../b311) a anglicky (../b314)
#   bash regrese.sh PRAC JAVA_HOME
set -u
PRAC=$1; JV=$2
R=$(cd "$(dirname "$0")/../.." && pwd)
B=$R/test_overeni
OUT=$B/b316/VYSTUP_regrese.txt
B315=0fd67c7
P=$PRAC/reg316
mkdir -p $P
ok=0; ch=0
radek() { echo "$1" >> $OUT; }
vysl() {   # vysl "popis" ok(0/1) "detail"
  if [ "$2" = "1" ]; then ok=$((ok+1)); radek "OK     $1  -- $3"; else ch=$((ch+1)); radek "CHYBA  $1  -- $3"; fi
}
echo "B316 - regrese: jadro Atari bez cartridge = B315 (git $B315), testy C++ s vlakny, navod" > $OUT

# ---------- 1) test_core B315 vs B316 ----------
rm -rf $P/b315 && mkdir -p $P/b315 && (cd $R && git archive $B315 app/src/main/cpp/atari) | tar -x -C $P/b315
OLD=$P/b315/app/src/main/cpp/atari
NEW=$R/app/src/main/cpp/atari
g++ -std=c++17 -O2 -I$OLD -o $P/core_b315 $B/b292/test_core.cpp &
g++ -std=c++17 -O2 -I$NEW -o $P/core_b316 $B/b292/test_core.cpp &
wait
if [ ! -f $P/acid800.atr ]; then
  curl -sSfL -o $P/acid800.atr "https://raw.githubusercontent.com/atari800/atari800/98cc2dc749867c9c7317a0271ba062a6c797f886/test/acid800.atr" || rm -f $P/acid800.atr
fi
# casy behu na PC (us / ms) se lisi beh od behu - pryc; vse ostatni musi sedet do bajtu
norm() { sed -E 's/[0-9]+(\.[0-9]+)? ?(us|ms)\b/<cas>/g' "$1"; }
spust() {   # spust jmeno "argumenty" [promenne prostredi]
  local jm=$1 args=$2 envs=${3:-}
  for v in b315 b316; do
    rm -rf $P/run_$v/$jm && mkdir -p $P/run_$v/$jm
    (cd $P/run_$v/$jm && env $envs timeout 1200 $P/core_$v $args > out.txt 2>&1; echo "navrat $?" >> out.txt)
  done
  local d=$(diff <(norm $P/run_b315/$jm/out.txt) <(norm $P/run_b316/$jm/out.txt) | grep -c '^[<>]')
  local sn=0 snr=0
  for f in $(cd $P/run_b315/$jm && ls *.idx *.ppm *.wav 2>/dev/null); do
    sn=$((sn+1))
    cmp -s $P/run_b315/$jm/$f $P/run_b316/$jm/$f || snr=$((snr+1))
  done
  local posl=$(grep -v '^navrat' $P/run_b316/$jm/out.txt | grep -v '^ *$' | tail -1 | cut -c1-110)
  if [ "$d" = "0" ] && [ "$snr" = "0" ]; then
    vysl "test_core $jm" 1 "vystup B316 = B315 ($(wc -l < $P/run_b316/$jm/out.txt) radku), souboru obrazu/WAV $sn shodnych; $posl"
  else
    vysl "test_core $jm" 0 "rozdilnych radku $d, rozdilnych souboru $snr z $sn"
    diff <(norm $P/run_b315/$jm/out.txt) <(norm $P/run_b316/$jm/out.txt) | head -20 >> $OUT
  fi
}
spust boot "boot"
spust xe130 "xe130"
spust pokey-casovac "pokey-casovac"
spust vlozeni "vlozeni"
spust tv-proud "tv-proud"
spust pc-klavesnice "pc-klavesnice"
spust pc-klavesnice-naivne "pc-klavesnice naivne"
spust kazeta "kazeta csave_test.wav"
spust kazeta-dlouha-basic "kazeta-dlouha basic 12"
spust kazeta-dlouha-tbxl "kazeta-dlouha tbxl 10"
spust bootkazeta "bootkazeta synt"
spust bootkazeta-stereo "bootkazeta synt-stereo"
spust txt-basic "txt-program zkouska basic"
spust txt-tbxl "txt-program zkouska tbxl"
spust tbxl-disketa "tbxl-disketa"
spust tbxl-disketa-stary "tbxl-disketa stary"
if [ -f $P/acid800.atr ]; then spust acid800 "acid $P/acid800.atr"; else radek "(Acid800 preskocen - acid800.atr se nepodarilo stahnout)"; fi
ln -sf "$R/test_assets/Popeye (VBXE, PAL Version)(2).xex" $P/popeye.xex
ln -sf "$R/test_assets/wolf3d(1).atr" $P/wolf3d.atr
spust popeye-vbxe "xex $P/popeye.xex 2600 pop 1500,2600" "KLAVESY=400:33,1600:1000,1800:1001 JOY=1900-2600:4"
spust wolf3d-vbxe "atr $P/wolf3d.atr 1700 w3d 300,1400,1560,1700" "KLAVESY=400:33,700:12,900:12,1100:12,1570:33 JOY=1500-1560:1"
i=0
for x in "$R"/test_assets/*.xex; do
  case "$x" in *Popeye*) continue;; esac
  i=$((i+1))
  ln -sf "$x" $P/hra$i.xex
  spust "xex$i-$(basename "$x" .xex | tr -c 'A-Za-z0-9\n' '_')" "xex $P/hra$i.xex 1500 hra 300,800,1500"
done

# ---------- 2) test_b291_runtime B315 vs B316 ----------
g++ -std=c++17 -O2 -I$OLD -o $P/rt_b315 $B/b291/test_b291_runtime.cpp &
g++ -std=c++17 -O2 -I$NEW -o $P/rt_b316 $B/b291/test_b291_runtime.cpp &
wait
(cd $P && ./rt_b315 > rt_b315.txt 2>&1; ./rt_b316 > rt_b316.txt 2>&1)
d=$(diff <(norm $P/rt_b315.txt) <(norm $P/rt_b316.txt) | grep -c '^[<>]')
vysl "test_b291_runtime (psani klavesnici, CSAVE -> WAV)" $([ "$d" = "0" ] && echo 1 || echo 0) "B316 = B315: rozdilnych radku $d; $(tail -1 $P/rt_b316.txt)"

# ---------- 3) skutecny nap_atari_native.cpp s vlakny ----------
J="-I$B/b291/stub -I$JV/include -I$JV/include/linux -I$NEW"
(cd $B/b291 && g++ -std=c++17 -O2 -pthread $J -o $P/jni291 test_b291_jni_host.cpp 2>/dev/null && cd $P && \
  timeout 900 ./jni291 $R/app/src/main/assets/emu_atari_cpp/turbo_basic_xl.xex > jni291.txt 2>&1)
t=$(grep "VYSLEDEK" $P/jni291.txt | tail -1)
vysl "B291 test_b291_jni_host (pristroj, psani, CSAVE)" $(echo "$t" | grep -q " 0 chyb" && echo 1 || echo 0) "$t"
(cd $B/b292 && g++ -std=c++17 -O2 -pthread -I$B/b291/stub -I$JV/include -I$JV/include/linux -I$NEW -o $P/jni292 test_b292_jni_host.cpp 2>/dev/null && \
  cd $B/b292 && ATR=$P/acid800.atr timeout 1500 $P/jni292 > $P/jni292.txt 2>&1)
t=$(grep "VYSLEDEK" $P/jni292.txt | tail -1)
vysl "B292-B303 test_b292_jni_host (kazeta, disketa, joystick, VBXE Popeye + W3D, TV, klavesnice PC, Ctrl+V)" $(echo "$t" | grep -q " 0 chyb" && echo 1 || echo 0) "$t"

# ---------- 4) navod v appce (napoveda/index.html + en.html) ----------
(cd $B/b312 && python3 kontrola_navodu.py > $P/navod_cz.txt 2>&1)
t=$(grep "VYSLEDEK" $P/navod_cz.txt | tail -1)
vysl "navod cesky: kazde tlacitko z navodu v appce opravdu je (../b312 kontrola_navodu.py)" $(echo "$t" | grep -q " 0 CHYBA" && echo 1 || echo 0) "$t"
(cd $B/b314 && python3 kontrola_navodu_en.py > $P/navod_en.txt 2>&1)
t=$(grep "VYSLEDEK" $P/navod_en.txt | tail -1)
vysl "navod anglicky: tlacitka v appce anglicky, zadna cestina, stejne kapitoly (../b314 kontrola_navodu_en.py)" $(echo "$t" | grep -q " 0 CHYB" && echo 1 || echo 0) "$t"
#    B314 navod v Chromiu: kontrola 5) "cesky navod = B313 + zmeny B314" v B316 zamerne neplati (pribyla cartridge
#    v XEX / MOBIL) - povolena jen ta, a jen s novymi texty B316; co presne se zmenilo, hlida kontrola_navodu_b316.py
(cd $B/b314 && timeout 900 python3 test_b314_jazyk_navod.py $P/navod > $P/navod_b314.txt 2>&1)
t=$(grep "VYSLEDEK" $P/navod_b314.txt | tail -1)
jine=$(grep "^CHYBA" $P/navod_b314.txt | grep -v "^CHYBA  5) cesky navod: texty jako v B313" | wc -l)
nec=$(grep "^CHYBA  5) cesky navod" $P/navod_b314.txt | sed -n "s/.*NECEKANE: //p")
spr=$(python3 -c "
import ast, sys
s = sys.argv[1].strip()
v = ast.literal_eval(s) if s else []
ok = {'VYSUNOUT CARTRIDGE', '(Atari znovu s BASICem) a', 'VYBRAT JINÝ SOUBOR'}
print(1 if set(v) <= ok else 0)" "$nec")
vysl "B314 navod v Chromiu (presmerovani CZ/EN, hledani, 320-915 px, bez chyb JS; 5) jen nove texty B316: $nec)" \
  $([ "$jine" = "0" ] && [ "$spr" = "1" ] && echo 1 || echo 0) "$t, jinych chyb nez 5): $jine"
(cd $R && git checkout -- test_overeni/b311 test_overeni/b312 test_overeni/b314 2>/dev/null)

# uplne vystupy testu s vlakny a navodu sem (cesty PRAC zkracene)
for x in "jni291:b291_jni_host" "jni292:b292_jni_host" "navod_cz:navod_cz_b312" "navod_en:navod_en_b314" "navod_b314:b314_navod_chromium"; do
  sed "s#$PRAC#PRAC#g" $P/${x%%:*}.txt > $B/b316/VYSTUP_regrese_${x##*:}.txt 2>/dev/null
done

radek "VYSLEDEK: $ok OK, $ch CHYBA"
tail -1 $OUT
