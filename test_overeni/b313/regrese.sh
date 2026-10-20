#!/bin/bash
# B313: regrese po zmenach rozbocovace a kodeku JPEG (nap_tv_proud*.h), konzoli (intro i menu)
# a menu - starsi testy beze zmeny, vysledky do VYSTUP_regrese.txt (puvodni VYSTUP_* starsich
# verzi se neprepisuji).
#   bash regrese.sh PRAC INC JAVA_HOME
set -u
PRAC=$1; INC=$2; JV=$3
R=$(cd "$(dirname "$0")/../.." && pwd)
B=$R/test_overeni
export EGL_PLATFORM=surfaceless
TV="-I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb"
OUT=$B/b313/VYSTUP_regrese.txt
mkdir -p $PRAC/reg
ok=0; ch=0
radek() { echo "$1" >> $OUT; }
vysl() {   # vysl "popis" "posledni radek vystupu" "vzor pro OK"
  if echo "$2" | grep -q "$3"; then ok=$((ok+1)); radek "OK     $1  -- $2"; else ch=$((ch+1)); radek "CHYBA  $1  -- $2"; fi
}
echo "B313 - regrese (starsi testy na kodu B313)" > $OUT

# snimky 3D sceny pro kodeky PS1 (jako ../b305 krok 0: demo pres Mesa, 320x240 a 640x480)
(cd $B/b307 && g++ -std=c++17 -O2 -w -I$INC -o $PRAC/reg/demo_snimky demo_snimky.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz)
if [ ! -d $PRAC/reg/png320 ]; then
  mkdir -p $PRAC/reg/png320 $PRAC/reg/png640
  T320=$(python3 -c "print(' '.join('%.4f'%(a+i/30) for a in (20.0,10.0,29.0) for i in range(60 if a==20.0 else 30)))")
  T640=$(python3 -c "print(' '.join('%.4f'%(20.0+i/30) for i in range(30)))")
  (cd $B/b307 && $PRAC/reg/demo_snimky $PRAC/reg/png320 320 240 $T320 > /dev/null 2>&1; $PRAC/reg/demo_snimky $PRAC/reg/png640 640 480 $T640 > /dev/null 2>&1)
fi

# 1) B305: kodeky Sega / PS1, koder na vic jadrech, rozbocovac (+ TSan)
cd $B/b305
g++ -std=c++17 -O2 -pthread -w $TV -I. -o $PRAC/reg/test_kodeky test_kodeky.cpp && $PRAC/reg/test_kodeky $PRAC/reg/png320 $PRAC/reg/png640 > $PRAC/reg/kodeky.txt 2>&1
vysl "B305 kodeky Sega (bez ztraty) a PS1 (JPEG po castech)" "$(tail -1 $PRAC/reg/kodeky.txt)" "0 chyb"
g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -o $PRAC/reg/test_pasy test_pasy.cpp && $PRAC/reg/test_pasy > $PRAC/reg/pasy.txt 2>&1
vysl "B305 koder JPEG na vic jadrech (bajt po bajtu stejne)" "$(tail -1 $PRAC/reg/pasy.txt)" "OK"
g++ -std=c++17 -O2 -pthread -w $TV -I. -o $PRAC/reg/test_hub test_hub.cpp && $PRAC/reg/test_hub $PRAC/reg/png320 > $PRAC/reg/hub.txt 2>&1
vysl "B305 rozbocovac (jadro 60/s, zvuk, rychly a pomaly prohlizec)" "$(tail -1 $PRAC/reg/hub.txt)" "0 chyb"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -w $TV -I. -o $PRAC/reg/test_hub_tsan test_hub.cpp && NAP_TSAN=1 TSAN_OPTIONS="halt_on_error=0" timeout 900 $PRAC/reg/test_hub_tsan $PRAC/reg/png320 > $PRAC/reg/hub_tsan.txt 2>&1
vysl "B305 rozbocovac pod ThreadSanitizerem" "$(tail -1 $PRAC/reg/hub_tsan.txt), hlaseni TSan $(grep -c 'WARNING: ThreadSanitizer' $PRAC/reg/hub_tsan.txt)" "0 chyb, hlaseni TSan 0"

# 2) B306: pakety jen se zvukem
cd $B/b306
g++ -std=c++17 -O2 -pthread $TV -I../b305 -o $PRAC/reg/test_hub6 test_hub6.cpp && $PRAC/reg/test_hub6 > $PRAC/reg/hub6.txt 2>&1
vysl "B306 rozbocovac: pakety jen se zvukem" "$(tail -1 $PRAC/reg/hub6.txt)" "0 chyb"

# 3) B309: SKUTECNY nap_intro_gl.cpp (nove konzole s pohyblivymi dily), cela stopa bit po bitu
cd $B/b309
INTRO="-I../b306/stub -I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/intro"
mkdir -p $PRAC/reg/assety_bez_klipu/design
ln -sf $R/app/src/main/assets/design/nap_logo_original.png $PRAC/reg/assety_bez_klipu/design/nap_logo_original.png
g++ -std=c++17 -O2 -pthread -Wno-unused-function $INTRO -o $PRAC/reg/test_intro_host test_intro_host.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 && \
  timeout 900 $PRAC/reg/test_intro_host $PRAC/reg/assety_bez_klipu 2>&1 | grep -v "^DEMO\|^NAP_DEMO" > $PRAC/reg/intro_host.txt
vysl "B309 uvodni demo z Javy (JNI, OpenSL, SKIP, cela stopa)" "$(tail -1 $PRAC/reg/intro_host.txt)" "0 CHYB"

# 4) cele demo po 0,25 s (konzole N&P ve scene 4 s animacemi)
mkdir -p $PRAC/reg/vse
(cd $B/b307 && timeout 1500 $PRAC/reg/demo_snimky $PRAC/reg/vse 854 480 --vse 0.25 > $PRAC/reg/vse.txt 2>&1)
vysl "B307 cele demo po 0,25 s (854x480)" "snimku $(grep -c '^t=' $PRAC/reg/vse.txt), chyb GL $(grep -c 'GL chyba' $PRAC/reg/vse.txt)" "snimku 237, chyb GL 0"

# 5) B311: menu - logika karuselu, snimky (telefon, na sirku, TV 1280x720), obrazky karet pro HTML
cd $B/b311
g++ -std=c++17 -O2 -I../../app/src/main/cpp/menu -o $PRAC/reg/test_karusel test_karusel.cpp && $PRAC/reg/test_karusel > $PRAC/reg/karusel.txt 2>&1
vysl "B311 logika karuselu (karty.txt, fyzika prstu, rozlozeni, klavesy)" "$(tail -1 $PRAC/reg/karusel.txt)" "0 CHYB\|0 chyb"
g++ -std=c++17 -O2 -w -I$INC -o $PRAC/reg/menu_snimky menu_snimky.cpp -l:libEGL.so.1 -l:libGLESv2.so.2
mkdir -p $PRAC/reg/menu
{ timeout 900 $PRAC/reg/menu_snimky $PRAC/reg/menu 1080 2340 0:0.35 0:3 1:3 1:6 2:9 2:12 3:3 6:3
  timeout 600 $PRAC/reg/menu_snimky $PRAC/reg/menu 1080 2340 --zamky 1:3 2:3
  timeout 600 $PRAC/reg/menu_snimky $PRAC/reg/menu 2340 1080 1:3 2:3
  timeout 600 $PRAC/reg/menu_snimky $PRAC/reg/menu 1280 720 --tv 1:3 1:6 2:9 2:12
  timeout 600 $PRAC/reg/menu_snimky $PRAC/reg/menu 600 800 --karty
  timeout 600 $PRAC/reg/menu_snimky $PRAC/reg/menu 1080 2340 --vykon 6; } > $PRAC/reg/menu.txt 2>&1
vysl "B311 snimky menu (na vysku, zamky, na sirku, TV 1280x720, karty, vykon)" "snimku $(grep -c '^snimek' $PRAC/reg/menu.txt), CHYB GL celkem nenulovych $(grep 'CHYB GL celkem' $PRAC/reg/menu.txt | grep -vc 'celkem: 0'), $(grep VYKON $PRAC/reg/menu.txt | head -1)" "nenulovych 0"
cp VYSTUP_kontrola_navodu.txt $PRAC/reg/navod_puvodni.txt
python3 kontrola_navodu.py > /dev/null 2>&1
vysl "B311 nazvy tlacitek v navodu proti appce (navod B313)" "$(tail -1 VYSTUP_kontrola_navodu.txt)" " 0 \|0 nenalez\|0 CHYB\|: 0"
cp VYSTUP_kontrola_navodu.txt $B/b313/VYSTUP_kontrola_navodu_b313.txt
cp $PRAC/reg/navod_puvodni.txt VYSTUP_kontrola_navodu.txt

# 6) TV / PC se strankou B313 (pollFps, 409, priznak 8): klavesnice PC do Atari (B302), Atari na TV + Ctrl+V (B303)
#    - testy pisi do svych adresaru: vysledek sem, original zpet
ATARI="-I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari"
(cd $B/b302 && cp VYSTUP_test_b302_prohlizec.txt $PRAC/reg/b302_puvodni.txt && \
  g++ -std=c++17 -O2 -shared -fPIC -I../../app/src/main/cpp/atari -o libpcklav.so pc_klav_lib.cpp && \
  timeout 900 python3 test_b302_prohlizec.py > $PRAC/reg/b302_log.txt 2>&1; \
  cp VYSTUP_test_b302_prohlizec.txt $B/b313/VYSTUP_b302_klavesnice_pc_se_strankou_b313.txt; \
  cp $PRAC/reg/b302_puvodni.txt VYSTUP_test_b302_prohlizec.txt; rm -f libpcklav.so)
vysl "B302 klavesnice PC do Atari se strankou B313" "$(tail -n 1 $B/b313/VYSTUP_b302_klavesnice_pc_se_strankou_b313.txt)" " 0 CHYB\| 0 chyb"
(cd $B/b303 && cp VYSTUP_test_b303_prohlizec.txt $PRAC/reg/b303_puvodni.txt && \
  g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $ATARI -o libtvproud.so tv_lib.cpp && \
  timeout 900 python3 test_b303_prohlizec.py > $PRAC/reg/b303_log.txt 2>&1; \
  cp VYSTUP_test_b303_prohlizec.txt $B/b313/VYSTUP_b303_atari_tv_se_strankou_b313.txt; \
  cp $PRAC/reg/b303_puvodni.txt VYSTUP_test_b303_prohlizec.txt; rm -f libtvproud.so)
vysl "B303 Atari na TV (proud z C++, Ctrl+V) se strankou B313" "$(tail -n 1 $B/b313/VYSTUP_b303_atari_tv_se_strankou_b313.txt)" " 0 CHYB\| 0 chyb"

radek "VYSLEDEK: $ok OK, $ch CHYBA"
