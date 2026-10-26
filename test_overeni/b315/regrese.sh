#!/bin/bash
# B315: regrese - testy stranky WEB TV starsich verzi se strankou B315 (zmenila se jen zvukova cast
# stranky v MainActivity.napTvWebWriteHtml). Vysledky sem (VYSTUP_regrese*.txt), puvodni VYSTUP_*
# starsich verzi se vrati z gitu (git checkout), aby zustaly, jak byly ve sve verzi.
#   bash regrese.sh PRAC INC JAVA_HOME PNG320
#   (PNG320 = snimky 3D sceny 320x240 jako ../b305/JAK_SPUSTIT.txt krok 0, pro test B306)
set -u
PRAC=$1; INC=$2; JV=$3; PNG320=$4
R=$(cd "$(dirname "$0")/../.." && pwd)
B=$R/test_overeni
OUT=$B/b315/VYSTUP_regrese.txt
mkdir -p $PRAC/reg315
export EGL_PLATFORM=surfaceless
# starsi testy pocitaji s ceskou appkou (viz ../b314/regrese.sh): cesky prohlizec = cesky uzivatel
export LC_MESSAGES=cs_CZ.UTF-8
ok=0; ch=0
radek() { echo "$1" >> $OUT; }
vysl() {   # vysl "popis" "vysledek" "vzor pro OK"
  if echo "$2" | grep -q "$3"; then ok=$((ok+1)); radek "OK     $1  -- $2"; else ch=$((ch+1)); radek "CHYBA  $1  -- $2"; fi
}
vrat() { (cd $R && git checkout -- "$@" 2>/dev/null); }
echo "B315 - regrese (testy stranky WEB TV z B302, B303, B306, B313 a B314 se strankou B315)" > $OUT

# 1) B313: prepnuti proudu po odchodu z menu (409 + X-Nap-Proud)
(cd $B/b313 && timeout 600 python3 test_b313_tv_prepnuti.py > $PRAC/reg315/b313_prepnuti.txt 2>&1)
cp $B/b313/VYSTUP_test_b313_tv_prepnuti.txt $B/b315/VYSTUP_regrese_b313_tv_prepnuti.txt
vysl "B313 TV / PC: prepnuti proudu po odchodu z menu (409 + X-Nap-Proud)" "$(tail -1 $B/b315/VYSTUP_regrese_b313_tv_prepnuti.txt)" " 0 CHYB"

# 2) B313: menu na TV / PC (SKUTECNE C++ menu, ostrost, kvalita, klavesnice - sipky listuji kartami)
JNI="-I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud"
(cd $B/b313 && g++ -std=c++17 -O2 -shared -fPIC -pthread -w $JNI -o $PRAC/libmenu_b313.so tv_menu_lib_b313.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 \
   && timeout 900 python3 test_b313_tv_menu.py $PRAC $PRAC/tv > $PRAC/reg315/b313_tv_menu.txt 2>&1)
cp $B/b313/VYSTUP_test_b313_tv_menu.txt $B/b315/VYSTUP_regrese_b313_tv_menu.txt
vysl "B313 menu na TV / PC v Chromiu (ostrost, kvalita, klavesnice)" "$(tail -1 $B/b315/VYSTUP_regrese_b313_tv_menu.txt)" " 0 CHYB"
vrat test_overeni/b313

# 3) B314: stranka WEB TV cesky = B313, anglicky bez cestiny (vsechny stavy)
#    kontrola a) "HTML stranky = B313 + jen napTr" v B315 zamerne neplati (zmenila se zvukova cast stranky -
#    co presne, hlida kontrola_stranky.py: B315 = B314 + jen zvuk) -> jedina povolena CHYBA; texty stranky
#    ve vsech stavech cesky = B313 a anglicky bez cestiny musi platit dal
(cd $B/b314 && timeout 1500 python3 test_b314_jazyk_tv.py > $PRAC/reg315/b314_tv.txt 2>&1)
cp $B/b314/VYSTUP_test_b314_jazyk_tv.txt $B/b315/VYSTUP_regrese_b314_jazyk_tv.txt
tv14=$(grep "^CHYBA" $B/b315/VYSTUP_regrese_b314_jazyk_tv.txt | grep -vc "a) cesky: stranka WEB TV z MainActivity = B313 + jen napTr")
vysl "B314 stranka WEB TV: texty cesky ve vsech stavech = B313, anglicky bez cestiny, bez chyby JS (HTML a) se v B315 zamerne zmenilo - zvuk)" \
  "$(tail -1 $B/b315/VYSTUP_regrese_b314_jazyk_tv.txt), jinych chyb nez HTML a): $tv14" "jinych chyb nez HTML a): 0"
vrat test_overeni/b314

# 4) B302: klavesnice PC do Atari 130XE (SKUTECNY C++ pristroje), 5) B303: Atari na TV (proud z C++) + Ctrl+V
(cd $B/b302 && g++ -std=c++17 -O2 -shared -fPIC -I../../app/src/main/cpp/atari -o libpcklav.so pc_klav_lib.cpp && \
  timeout 900 python3 test_b302_prohlizec.py > $PRAC/reg315/b302.txt 2>&1; \
  cp VYSTUP_test_b302_prohlizec.txt $B/b315/VYSTUP_regrese_b302_klavesnice_pc.txt; rm -f libpcklav.so)
vysl "B302 klavesnice PC do Atari se strankou B315" "$(tail -n 1 $B/b315/VYSTUP_regrese_b302_klavesnice_pc.txt)" " 0 CHYB\| 0 chyb"
vrat test_overeni/b302
ATARI="-I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari"
(cd $B/b303 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $ATARI -o libtvproud.so tv_lib.cpp && \
  timeout 900 python3 test_b303_prohlizec.py > $PRAC/reg315/b303.txt 2>&1; \
  cp VYSTUP_test_b303_prohlizec.txt $B/b315/VYSTUP_regrese_b303_atari_tv.txt; rm -f libtvproud.so)
vysl "B303 Atari na TV (proud z C++, zvuk, Ctrl+V) se strankou B315" "$(tail -n 1 $B/b315/VYSTUP_regrese_b303_atari_tv.txt)" " 0 CHYB\| 0 chyb"
vrat test_overeni/b303

# 6) B306: Sega, PS1 a demo na TV v Chromiu bez autoplay - zvuk (vyzva, kliknuti, F5, klavesa), Sega bod
#    po bodu 60/s 48000 Hz, zvuk za obrazem, navaznost kusu zvuku, zadrhavajici sit, PS1, demo.
#    Knihovny ze SOUCASNEHO C++ (jadro Segy clownmdemu jako v CMakeLists.txt); puvodni .so zpet.
V=$R/app/src/main/cpp/vendor/clownmdemu-core
CLOWN=$PRAC/clown
if [ ! -f $CLOWN/hotovo ]; then
  mkdir -p $CLOWN
  CF="-O2 -fPIC -w -DCC_USE_C99_INTEGERS=1 -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clown68000/source/common -I$V/libraries/clown68000/libraries/clowncommon -I$V/libraries/clownz80/source -I$V/libraries/clownz80/libraries/clowncommon"
  for f in bus-common bus-main-m68k bus-sub-m68k bus-z80 cdc cdda clownmdemu controller controller-manager controller-multitap-ea \
           controller-multitap-sega fm fm-channel fm-lfo fm-operator fm-phase io-port log low-pass-filter pcm psg sync vdp; do
    gcc $CF -c $V/source/$f.c -o $CLOWN/$f.o || echo CHYBA_CLOWN $f
  done
  gcc $CF -c $V/libraries/clown68000/source/common/opcode.c -o $CLOWN/m68k_opcode.o
  gcc $CF -c $V/libraries/clown68000/source/interpreter/clown68000.c -o $CLOWN/m68k_clown68000.o
  gcc $CF -c $V/libraries/clownz80/source/common.c -o $CLOWN/z80_common.o
  gcc $CF -c $V/libraries/clownz80/source/interpreter.c -o $CLOWN/z80_interpreter.o
  touch $CLOWN/hotovo
fi
mkdir -p $PRAC/reg315/so306 && cp $B/b306/*.so $PRAC/reg315/so306/ 2>/dev/null
SEGA="-DNAP_SEGA_VENDOR_CORE_PRESENT=1 -DCC_USE_C99_INTEGERS=1 -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clownz80/source"
PS1="-DANDROID -I../b305/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/vendor/stb"
LIBY=""
(cd $B/b306 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $SEGA -o libsega_b306.so tv_sega6.cpp $CLOWN/*.o -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2) > $PRAC/reg315/so_sega.txt 2>&1 && LIBY="$LIBY sega=soucasny" || LIBY="$LIBY sega=PREKLAD_SELHAL"
(cd $B/b306 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -o libps1_b306.so tv_ps1_6.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2) > $PRAC/reg315/so_ps1.txt 2>&1 && LIBY="$LIBY ps1=soucasny" || LIBY="$LIBY ps1=PREKLAD_SELHAL"
# demo: tv_intro.cpp z B306 jde prelozit jen s C++ dema z B306 (pozdeji zmizelo introZvukStopNative) -
# zustava knihovna z B306 (proud dema jen krmi stranku; stranka B315 je ta zkousena)
if [ -f $B/b306/libintro_b306.so ]; then LIBY="$LIBY intro=z_B306"; else LIBY="$LIBY intro=CHYBI"; fi
(cd $B/b306 && timeout 1200 python3 test_b306_prohlizec.py $PNG320 > $PRAC/reg315/b306.txt 2>&1)
cp $B/b306/VYSTUP_test_b306_prohlizec.txt $B/b315/VYSTUP_regrese_b306_prohlizec.txt
# kontrola "hlasi do logu kazdych 30 s" zavisi na nahode: hlaseni jde jen kdyz v okamziku 30s tiku bezi proud
# (pri prepnuti Sega -> PS1 nebo po konci dema ne - stejne se strankou B314); je-li to JEDINA chyba,
# test se jednou zopakuje (oba behy zustanou v souborech)
JINE6=$(grep "^CHYBA" $B/b315/VYSTUP_regrese_b306_prohlizec.txt | grep -vc "prohlizec hlasi do logu appky kazdych 30 s")
POZN6=""
if [ "$JINE6" = "0" ] && grep -q "^CHYBA" $B/b315/VYSTUP_regrese_b306_prohlizec.txt; then
  mv $B/b315/VYSTUP_regrese_b306_prohlizec.txt $B/b315/VYSTUP_regrese_b306_prohlizec_beh1.txt
  POZN6=" (1. beh: $(tail -1 $B/b315/VYSTUP_regrese_b306_prohlizec_beh1.txt) - jen 30s tik mimo proud; 2. beh:)"
  (cd $B/b306 && timeout 1200 python3 test_b306_prohlizec.py $PNG320 > $PRAC/reg315/b306_beh2.txt 2>&1)
  cp $B/b306/VYSTUP_test_b306_prohlizec.txt $B/b315/VYSTUP_regrese_b306_prohlizec.txt
fi
vysl "B306 Sega, PS1, demo na TV bez autoplay (zvuk: vyzva, kliknuti, F5, klavesa; zvuk za obrazem; zadrhavajici sit), knihovny:$LIBY$POZN6" \
  "$(tail -1 $B/b315/VYSTUP_regrese_b306_prohlizec.txt)" " 0 CHYB\| 0 chyb\|, 0 CHYBA"
cp $PRAC/reg315/so306/*.so $B/b306/ 2>/dev/null
vrat test_overeni/b306

radek "VYSLEDEK: $ok OK, $ch CHYBA"
