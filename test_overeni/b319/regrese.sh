#!/bin/bash
# B319: REGRESE - kodeky a rozbocovac proudu starsich verzi s hlavickami B319 (B305 kodeky / hub / pasy,
# B313 kodek menu), PS1 (testy B308 na kodu B319, JVM -Xcheck:jni, symboly, TV PS1 B305 / B306 preklad,
# javac proti android.jar, podpisy JNI), mys B318 na kodu B319 (logika + SKUTECNE jadro PCSX), STRANKA
# WEB TV starsich verzi se strankou B319 (zmenilo se: v=319 v adrese proudu, kopie a JPEG v polovicnim
# rozliseni, log, mys nejvys po 33 ms): B315 zvuk, B313 prepnuti proudu, B314 jazyk, B303 Atari + Ctrl+V,
# B302 klavesnice PC, B306 Sega / PS1 / demo bez autoplay; SKUTECNE hlavni menu (B313 / B318).
# Vysledky: VYSTUP_regrese.txt (souhrn) + VYSTUP_regrese_* (cele vystupy); puvodni VYSTUP_* starsich
# verzi se vrati z gitu.
#   bash regrese.sh PRAC INC JAVA_HOME ANDROID_JAR PNG320 PNG640
set -u
PRAC=$1; INC=$2; JV=$3; AJAR=${4:-}; PNG320=${5:-}; PNG640=${6:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
B=$R/test_overeni
Z=$B/b319
P=$PRAC/reg319
mkdir -p $P
OUT=$Z/VYSTUP_regrese.txt
export EGL_PLATFORM=surfaceless
export LC_MESSAGES=cs_CZ.UTF-8
PS1="-DANDROID -I../b305/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux"
TVP="-I$R/app/src/main/cpp/tvproud -I$R/app/src/main/cpp/vendor/stb -I."
ok=0; ch=0
radek() { echo "$1" >> $OUT; }
vysl() {   # vysl "popis" 1/0 "detail"
  if [ "$2" = "1" ]; then ok=$((ok+1)); radek "OK     $1  -- $3"; else ch=$((ch+1)); radek "CHYBA  $1  -- $3"; fi
}
vrat() { (cd $R && git checkout -- "$@" 2>/dev/null); }
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
nula() { echo "$1" | grep -q " 0 CHYB\| 0 chyb\|, 0 CHYBA\| 0 CHYBA\|VYSLEDEK: OK" && echo 1 || echo 0; }
echo "B319 - regrese: kodeky a Hub proudu (B305 / B313), PS1 (B308, JVM, JNI, javac), mys B318, stranka WEB TV starsich testu se strankou B319" > $OUT

# ---------- 0) kodeky a rozbocovac proudu starsich verzi s hlavickami B319 (bez voleb = jako drive) ----------
cd $B/b305
if [ -n "$PNG320" ] && [ -d "$PNG320" ] && [ -n "$PNG640" ] && [ -d "$PNG640" ]; then
  g++ -std=c++17 -O2 -pthread $TVP -o $P/test_kodeky test_kodeky.cpp && $P/test_kodeky $PNG320 $PNG640 2>&1 | cisty > $Z/VYSTUP_regrese_b305_kodeky.txt
  t=$(tail -1 $Z/VYSTUP_regrese_b305_kodeky.txt)
  vysl "B305 kodeky (Sega indexy bez ztraty, PS1 JPEG po castech: PSNR, stojici obraz, kvalita podle site, rozmery) s hlavickami B319" $(nula "$t") "$t"
  g++ -std=c++17 -O2 -pthread $TVP -o $P/test_hub test_hub.cpp && $P/test_hub $PNG320 2>&1 | cisty > $Z/VYSTUP_regrese_b305_hub.txt
  t=$(tail -1 $Z/VYSTUP_regrese_b305_hub.txt)
  vysl "B305 rozbocovac ve vlaknech (jadro 60 / s, zvuk, rychly a pomaly prohlizec) s hlavickami B319" $(nula "$t") "$t"
else
  radek "(B305 kodeky / hub preskoceny - PNG320 / PNG640 neni)"
fi
g++ -std=c++17 -O2 -pthread $TVP -o $P/test_pasy test_pasy.cpp && $P/test_pasy 2>&1 | cisty > $Z/VYSTUP_regrese_b305_pasy.txt
t=$(tail -1 $Z/VYSTUP_regrese_b305_pasy.txt)
vysl "B305 koder JPEG na vic jadrech (pasy 1 / 2 / 3, bajt po bajtu stejne) s B319 (barvy 4:4:4 volitelne)" $(nula "$t") "$t"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread $TVP -o $P/test_pasy_tsan test_pasy.cpp && $P/test_pasy_tsan > $P/pasy_tsan.txt 2>&1
n=$(grep -c 'WARNING: ThreadSanitizer' $P/pasy_tsan.txt)
vysl "B305 koder JPEG na vic jadrech pod ThreadSanitizerem" $([ "$n" = "0" ] && echo 1 || echo 0) "hlaseni ThreadSanitizer: $n, $(tail -1 $P/pasy_tsan.txt)"
cd $B/b313
g++ -std=c++17 -O2 -pthread $TVP -o $P/test_kodek_menu test_kodek_menu.cpp && $P/test_kodek_menu 2>&1 | cisty > $Z/VYSTUP_regrese_b313_kodek_menu.txt
t=$(tail -1 $Z/VYSTUP_regrese_b313_kodek_menu.txt)
vysl "B313 kodek hlavniho menu na TV (vic obdelniku, zjemneni za pohybu, glReadPixels) s hlavickami B319" $(nula "$t") "$t"
vrat test_overeni/b305 test_overeni/b313

# ---------- 1) testy B308 na kodu B319 ----------
cd $B/b308
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wall -Wno-unused-function $PS1 -o $P/libps1_b319.so ps1_harness.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 2> $P/preklad_harness.txt
vysl "preklad ps1_harness.cpp (B308) s nap_ps1_native.cpp B319, -Wl,--no-undefined" $([ -f $P/libps1_b319.so ] && echo 1 || echo 0) \
     "varovani: $(grep -c 'warning' $P/preklad_harness.txt), chyb: $(grep -c 'error' $P/preklad_harness.txt)"
rm -rf $P/denik
timeout 900 python3 test_ps1_denik.py $P/libps1_b319.so $P/denik > /dev/null 2>&1
cp VYSTUP_test_ps1_denik.txt $Z/VYSTUP_regrese_b308_test_ps1_denik.txt
t=$(tail -1 VYSTUP_test_ps1_denik.txt)
vysl "B308 denik PS1 + klavesnice z TV v C++ + zamek kolem retro_run (test_ps1_denik.py)" $(nula "$t") "$t"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $PS1 -o $P/tsan_denik tsan_denik.cpp -l:libEGL.so.1 -l:libGLESv2.so.2
rm -rf $P/tsan_prac
TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 detect_deadlocks=0" timeout 900 $P/tsan_denik $P/tsan_prac > $P/tsan_denik.txt 2>&1
n=$(grep -c 'WARNING: ThreadSanitizer' $P/tsan_denik.txt)
vysl "B308 ThreadSanitizer: vlakno emulace, zvuk, klavesy z prohlizece, denik, SAVE / LOAD, dotyk (tsan_denik.cpp)" \
     $([ "$n" = "0" ] && grep -q "^stop" $P/tsan_denik.txt && echo 1 || echo 0) "hlaseni ThreadSanitizer: $n"
mkdir -p $P/jvmlib $P/jvmout; rm -rf $P/jvm_prac $P/jvm318 $P/jvm319
cp $P/libps1_b319.so $P/jvmlib/libnapps1core.so
# TestB318Mys pocita native metody (28 v B318) - v B319 jich je 29 (+ proudOtevri2Native)
sed 's/nativnich == 28, "native metod ve tride: " + nativnich + " (23 z B316 + 4 mys B317 + ps1MysZrychleni B318)"/nativnich == 29, "native metod ve tride: " + nativnich + " (23 z B316 + 4 mys B317 + ps1MysZrychleni B318 + proudOtevri2Native B319)"/' \
    $B/b318/jvm/TestB318Mys.java > $P/TestB318Mys.java
javac -d $P/jvmout -sourcepath ../b306/jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java jvm/TestPodpisu.java \
    $P/TestB318Mys.java $Z/jvm/TestB319Proud.java 2>&1 | grep -v JAVA_TOOL
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestPodpisu $P/jvm_prac 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$" | cisty > $Z/VYSTUP_regrese_b308_TestPodpisu.txt
t=$(tail -1 $Z/VYSTUP_regrese_b308_TestPodpisu.txt)
vysl "B308 JVM TestPodpisu (-Xcheck:jni): skutecna trida NativePs1CoreBridge B319" $(nula "$t") "$t"
timeout 900 python3 test_b308_klavesnice.py $P/libps1_b319.so $P/klavesnice > /dev/null 2>&1
cp VYSTUP_test_b308_klavesnice.txt $Z/VYSTUP_regrese_b308_test_b308_klavesnice.txt
t=$(tail -1 VYSTUP_test_b308_klavesnice.txt)
vysl "B308 klavesnice PC v TV prohlizeci -> PS1 (Chromium, stranka z MainActivity.java B319)" $(nula "$t") "$t"
LANG=cs_CZ.UTF-8 LANGUAGE=cs timeout 900 python3 test_b308_ps1_stranka.py > /dev/null 2>&1
cp VYSTUP_test_b308_ps1_stranka.txt $Z/VYSTUP_regrese_b308_test_b308_ps1_stranka.txt
t=$(tail -1 VYSTUP_test_b308_ps1_stranka.txt)
vysl "B308 stranka PS1: nahravani hry, banner, LOG s denikem (Chromium cesky)" $(nula "$t") "$t"
vrat test_overeni/b308

# ---------- 2) JVM: proud B319 (proudOtevri2) a mys B318 na kodu B319, symboly ----------
cd $Z
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestB319Proud $P/jvm319 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$\|^BUILD2S\|PIXFMT" | cisty > $Z/jvm/VYSTUP_TestB319Proud.txt
t=$(tail -1 $Z/jvm/VYSTUP_TestB319Proud.txt)
vysl "JVM TestB319Proud (-Xcheck:jni): proudOtevri2Native(int) primo i pres obal, proud z bezici hry B319 i staremu prohlizeci, log Hubu" \
     $(echo "$t" | grep -q " 0 chyb" && echo 1 || echo 0) "$t"
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestB318Mys $P/jvm318 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$\|^BUILD2S\|PIXFMT" | cisty > $Z/jvm/VYSTUP_TestB318Mys_na_B319.txt
t=$(tail -1 $Z/jvm/VYSTUP_TestB318Mys_na_B319.txt)
vysl "JVM TestB318Mys (-Xcheck:jni) na kodu B319: mys PS1 (B317 / B318) beze zmeny" $(echo "$t" | grep -q " 0 chyb" && echo 1 || echo 0) "$t"
{
  for m in $(grep -oE "native [][a-zA-Z]+ [a-zA-Z0-9_]+\(" $R/app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java | awk '{print $3}' | tr -d '('); do
    if nm -D --defined-only $P/libps1_b319.so | grep -q " Java_eu_atarihelp_emu10_NativePs1CoreBridge_${m}$"; then echo "OK     $m"; else echo "CHYBA  $m (v knihovne neni)"; fi
  done
} > $Z/jvm/VYSTUP_symboly.txt
vysl "kazda native metoda NativePs1CoreBridge ma v knihovne svou funkci (nm)" $([ "$(grep -c '^CHYBA' $Z/jvm/VYSTUP_symboly.txt)" = "0" ] && echo 1 || echo 0) \
     "$(grep -c '^OK' $Z/jvm/VYSTUP_symboly.txt) OK, $(grep -c '^CHYBA' $Z/jvm/VYSTUP_symboly.txt) CHYB"

# ---------- 3) testy TV PS1 B305 / B306: preklad + link ----------
cd $B/b305
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -I$R/app/src/main/cpp/vendor/stb -o $P/libps1_b305.so tv_ps1.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 > $P/preklad_b305.txt 2>&1
vysl "B305 tv_ps1.cpp (proud PS1 na TV) s nap_ps1_native.cpp B319: preklad + link" $([ -f $P/libps1_b305.so ] && echo 1 || echo 0) "$(tail -1 $P/preklad_b305.txt | cut -c1-120)"
cd $B/b306
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -I$R/app/src/main/cpp/vendor/stb -o $P/libps1_b306.so tv_ps1_6.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 > $P/preklad_b306.txt 2>&1
vysl "B306 tv_ps1_6.cpp s nap_ps1_native.cpp B319: preklad + link" $([ -f $P/libps1_b306.so ] && echo 1 || echo 0) "$(tail -1 $P/preklad_b306.txt | cut -c1-120)"

# ---------- 4) javac proti android.jar + podpisy JNI NativePs1CoreBridge ----------
if [ -n "$AJAR" ] && [ -f "$AJAR" ]; then
  rm -rf $P/javac_out && mkdir -p $P/javac_out
  (cd $R && timeout 900 $JV/bin/javac -proc:none --release 17 -cp "$AJAR" -Xmaxerrs 1000 -nowarn -d $P/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java > $P/javac.txt 2>&1)
  n=$(grep -c "error:" $P/javac.txt)
  {
    echo "B319 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    grep -v JAVA_TOOL $P/javac.txt
    echo "trid prelozeno: $(find $P/javac_out -name '*.class' | wc -l)"
    echo "VYSLEDEK: $n chyb prekladu"
  } > $Z/VYSTUP_javac_android.txt
  vysl "javac proti android.jar (cela appka)" $([ "$n" = "0" ] && echo 1 || echo 0) "$(tail -2 $Z/VYSTUP_javac_android.txt | tr '\n' ' ')"
  rm -rf $P/jnih && mkdir -p $P/jnih/h $P/jnih/out
  (cd $R && $JV/bin/javac -proc:none --release 17 -cp "$AJAR" -h $P/jnih/h -d $P/jnih/out \
      app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java 2>&1 | grep -v JAVA_TOOL)
  python3 $Z/kontrola_jni_ps1.py $P/jnih/h > /dev/null
  t=$(tail -1 $Z/VYSTUP_kontrola_jni_ps1.txt)
  vysl "podpisy JNI: NativePs1CoreBridge (javac -h) = nap_ps1_native.cpp (jmeno, navratovy typ, typy parametru)" \
       $(echo "$t" | grep -q " 0 CHYB" && echo 1 || echo 0) "$t"
else
  radek "(javac a podpisy JNI preskoceny - ANDROID_JAR neni)"
fi

# ---------- 5) stranka WEB TV: testy starsich verzi se strankou B319 ----------
(cd $B/b315 && timeout 1800 python3 test_b315_zvuk_tv.py > $P/b315.txt 2>&1)
cp $B/b315/VYSTUP_test_b315_zvuk_tv.txt $Z/VYSTUP_regrese_b315_zvuk_tv.txt
t=$(tail -1 $Z/VYSTUP_regrese_b315_zvuk_tv.txt)
vysl "B315 zvuk na TV / PC v dalsich hrach (menu -> Atari -> Sega -> PS1 -> MP3 ...), stranka B319" $(nula "$t") "$t"
vrat test_overeni/b315
(cd $B/b313 && timeout 600 python3 test_b313_tv_prepnuti.py > $P/b313.txt 2>&1)
cp $B/b313/VYSTUP_test_b313_tv_prepnuti.txt $Z/VYSTUP_regrese_b313_tv_prepnuti.txt
t=$(tail -1 $Z/VYSTUP_regrese_b313_tv_prepnuti.txt)
vysl "B313 TV / PC: prepnuti proudu po odchodu z menu (409 + X-Nap-Proud), stranka B319" $(nula "$t") "$t"
vrat test_overeni/b313
(cd $B/b314 && timeout 1500 python3 test_b314_jazyk_tv.py > $P/b314_tv.txt 2>&1)
cp $B/b314/VYSTUP_test_b314_jazyk_tv.txt $Z/VYSTUP_regrese_b314_jazyk_tv.txt
tv14=$(grep "^CHYBA" $Z/VYSTUP_regrese_b314_jazyk_tv.txt | grep -vc "a) cesky: stranka WEB TV z MainActivity = B313 + jen napTr")
vysl "B314 stranka WEB TV: texty cesky ve vsech stavech = B313, anglicky bez cestiny, bez chyby JS (HTML a) zamerne jine od B315)" \
     $([ "$tv14" = "0" ] && echo 1 || echo 0) "$(tail -1 $Z/VYSTUP_regrese_b314_jazyk_tv.txt), jinych chyb nez HTML a): $tv14"
vrat test_overeni/b314
(cd $B/b302 && g++ -std=c++17 -O2 -shared -fPIC -I../../app/src/main/cpp/atari -o libpcklav.so pc_klav_lib.cpp && \
  timeout 900 python3 test_b302_prohlizec.py > $P/b302.txt 2>&1; \
  cp VYSTUP_test_b302_prohlizec.txt $Z/VYSTUP_regrese_b302_klavesnice_pc.txt; rm -f libpcklav.so)
t=$(tail -n 1 $Z/VYSTUP_regrese_b302_klavesnice_pc.txt)
vysl "B302 klavesnice PC do Atari se strankou B319" $(nula "$t") "$t"
vrat test_overeni/b302
ATARI="-I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari"
(cd $B/b303 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $ATARI -o libtvproud.so tv_lib.cpp && \
  timeout 900 python3 test_b303_prohlizec.py > $P/b303.txt 2>&1; \
  cp VYSTUP_test_b303_prohlizec.txt $Z/VYSTUP_regrese_b303_atari_tv.txt; rm -f libtvproud.so)
t=$(tail -n 1 $Z/VYSTUP_regrese_b303_atari_tv.txt)
vysl "B303 Atari na TV (proud z C++, zvuk, Ctrl+V) se strankou B319" $(nula "$t") "$t"
vrat test_overeni/b303
if [ -n "$PNG320" ] && [ -d "$PNG320" ]; then
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
  mkdir -p $P/so306 && cp $B/b306/*.so $P/so306/ 2>/dev/null
  SEGA="-DNAP_SEGA_VENDOR_CORE_PRESENT=1 -DCC_USE_C99_INTEGERS=1 -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clownz80/source"
  LIBY=""
  (cd $B/b306 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $SEGA -o libsega_b306.so tv_sega6.cpp $CLOWN/*.o -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2) > $P/so_sega.txt 2>&1 && LIBY="$LIBY sega=soucasny" || LIBY="$LIBY sega=PREKLAD_SELHAL"
  (cd $B/b306 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -I$R/app/src/main/cpp/vendor/stb -o libps1_b306.so tv_ps1_6.cpp -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2) > $P/so_ps1.txt 2>&1 && LIBY="$LIBY ps1=soucasny" || LIBY="$LIBY ps1=PREKLAD_SELHAL"
  if [ -f $B/b306/libintro_b306.so ]; then LIBY="$LIBY intro=z_B306"; else LIBY="$LIBY intro=CHYBI"; fi
  (cd $B/b306 && timeout 1500 python3 test_b306_prohlizec.py $PNG320 > $P/b306.txt 2>&1)
  cp $B/b306/VYSTUP_test_b306_prohlizec.txt $Z/VYSTUP_regrese_b306_prohlizec.txt
  JINE6=$(grep "^CHYBA" $Z/VYSTUP_regrese_b306_prohlizec.txt | grep -vc "prohlizec hlasi do logu appky kazdych 30 s")
  POZN6=""
  if [ "$JINE6" = "0" ] && grep -q "^CHYBA" $Z/VYSTUP_regrese_b306_prohlizec.txt; then
    mv $Z/VYSTUP_regrese_b306_prohlizec.txt $Z/VYSTUP_regrese_b306_prohlizec_beh1.txt
    POZN6=" (1. beh jen 30s tik mimo proud; 2. beh:)"
    (cd $B/b306 && timeout 1500 python3 test_b306_prohlizec.py $PNG320 > $P/b306_beh2.txt 2>&1)
    cp $B/b306/VYSTUP_test_b306_prohlizec.txt $Z/VYSTUP_regrese_b306_prohlizec.txt
  fi
  t=$(tail -1 $Z/VYSTUP_regrese_b306_prohlizec.txt)
  vysl "B306 Sega, PS1, demo na TV bez autoplay (zvuk, F5, klavesa, zadrhavajici sit), knihovny:$LIBY$POZN6" $(nula "$t") "$t"
  cp $P/so306/*.so $B/b306/ 2>/dev/null
  vrat test_overeni/b306
else
  radek "(B306 preskocen - PNG320 neni)"
fi

# ---------- 6) SKUTECNE hlavni menu (nap_menu_gl.cpp, Mesa) - kodek menu z nap_tv_proud_jpeg.h B319 ----------
JNI="-I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud"
(cd $B/b313 && g++ -std=c++17 -O2 -pthread -w $JNI -o $P/test_menu_host_b313 test_menu_host_b313.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 \
   && cd $P && timeout 600 ./test_menu_host_b313 2>&1 | grep -v "^karta \|^DEMO" | cisty > $Z/VYSTUP_regrese_b313_menu_host.txt)
t=$(tail -1 $Z/VYSTUP_regrese_b313_menu_host.txt)
tvj=$(grep -c "TV: stara cesta pres Javu (prohlizec bez proudu) 1280x720" $Z/VYSTUP_regrese_b313_menu_host.txt)
tvn=$(grep -c "TV: nikdo se nediva - TV obraz se nekresli" $Z/VYSTUP_regrese_b313_menu_host.txt)
vysl "B313 / B318 SKUTECNE menu na PC (Mesa): vse jako drive, radek MENU 'TV: nikdo se nediva' / 'TV: stara cesta pres Javu'" \
     $([ "$(nula "$t")" = "1" ] && [ "$tvj" -ge 1 ] && [ "$tvn" -ge 1 ] && echo 1 || echo 0) "$t; radku TV nikdo $tvn, stara cesta $tvj"
vrat test_overeni/b313

# ---------- 7) mys PS1 B318 na kodu B319: logika + SKUTECNE jadro PCSX (nap_ps1_native.cpp B319) ----------
cd $B/b318
g++ -std=c++17 -O2 -Wall -Wextra -Wshadow -pthread -o $P/test_b318_mys test_b318_mys.cpp 2> $P/preklad_mys.txt
mkdir -p $P/mys && $P/test_b318_mys $P/mys 2>&1 | cisty > $Z/VYSTUP_regrese_b318_mys.txt
t=$(tail -1 $Z/VYSTUP_regrese_b318_mys.txt)
vysl "B318 logika mysi PS1 (nap_ps1_mys.h): lano, zrychleni, rozlozeni podle hry, gesta, PC, pamet" $(nula "$t") "$t"
if [ ! -f $PRAC/jadro/hotovo ] && [ -z "$(ls $PRAC/jadro/*.o 2>/dev/null)" ]; then
  bash stavba_jadra.sh $PRAC > $PRAC/stavba_jadra.txt 2>&1
fi
[ -f $PRAC/mys_test.exe ] || python3 udelej_exe.py $PRAC > /dev/null 2>&1
g++ -std=c++17 -O1 -g -pthread $PS1 -o $P/integrace_b318 integrace_b318.cpp $PRAC/jadro/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl 2> $P/preklad_integrace.txt
if [ -f $P/integrace_b318 ]; then
  mkdir -p $P/integr && (cd $P/integr && timeout 900 $P/integrace_b318 $P/integr $PRAC/mys_test.exe > $P/integrace.txt 2>&1)
  grep -E "^(B318|OK|CHYBA|VYSLEDEK)" $P/integrace.txt | cisty > $Z/VYSTUP_regrese_b318_integrace.txt
  t=$(tail -1 $Z/VYSTUP_regrese_b318_integrace.txt)
  vysl "B318 SKUTECNE jadro PS1 (PCSX z appky, BIOS HLE) + nap_ps1_native.cpp B319: mys v hre, ktera kresli / cte kazdy 3. snimek" $(nula "$t") "$t"
else
  vysl "B318 SKUTECNE jadro PS1 + nap_ps1_native.cpp B319: preklad" 0 "$(grep -m1 error $P/preklad_integrace.txt | cut -c1-160)"
fi
vrat test_overeni/b318

radek "VYSLEDEK: $ok OK, $ch CHYBA"
tail -1 $OUT
