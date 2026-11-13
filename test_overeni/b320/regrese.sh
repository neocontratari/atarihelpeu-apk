#!/bin/bash
# B320: REGRESE - vse z regrese B319 na kodu B320: kodeky a rozbocovac proudu starsich verzi s hlavickami
# B320 (B305 kodeky / hub / pasy, B313 kodek menu), PS1 (testy B308 na kodu B320, JVM -Xcheck:jni, symboly,
# TV PS1 B305 / B306 preklad, javac proti android.jar, podpisy JNI), mys B318 na kodu B320 (logika +
# SKUTECNE jadro PCSX), STRANKA WEB TV starsich verzi se strankou B320 (zmenilo se jen: window.NAP_VERZE,
# /status " verze=", stranka se po aktualizaci appky obnovi sama): B315 zvuk, B313 prepnuti proudu,
# B314 jazyk, B303 Atari + Ctrl+V, B302 klavesnice PC, B306 Sega / PS1 / demo bez autoplay; SKUTECNE
# hlavni menu (B313 / B318). NAVIC: vsechny testy B319 (kodek, Hub + TSan, stranka WEB TV s proudem B319,
# mys z PC, skripty stranky, jedno spojeni) na kodu B320 - kodek B320 bez ditheringu = B319.
# Vysledky: VYSTUP_regrese.txt (souhrn) + VYSTUP_regrese_* (cele vystupy); puvodni VYSTUP_* starsich
# verzi se vrati z gitu.
#   bash regrese.sh PRAC INC JAVA_HOME ANDROID_JAR PNG320 PNG640
set -u
PRAC=$1; INC=$2; JV=$3; AJAR=${4:-}; PNG320=${5:-}; PNG640=${6:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
B=$R/test_overeni
Z=$B/b320
P=$PRAC/reg320
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
echo "B320 - regrese: kodeky a Hub proudu (B305 / B313), PS1 (B308, JVM, JNI, javac), mys B318, stranka WEB TV starsich testu se strankou B320, testy B319 na kodu B320" > $OUT

# ---------- 0) kodeky a rozbocovac proudu starsich verzi s hlavickami B320 (bez voleb = jako drive) ----------
cd $B/b305
if [ -n "$PNG320" ] && [ -d "$PNG320" ] && [ -n "$PNG640" ] && [ -d "$PNG640" ]; then
  g++ -std=c++17 -O2 -pthread $TVP -o $P/test_kodeky test_kodeky.cpp && $P/test_kodeky $PNG320 $PNG640 2>&1 | cisty > $Z/VYSTUP_regrese_b305_kodeky.txt
  t=$(tail -1 $Z/VYSTUP_regrese_b305_kodeky.txt)
  vysl "B305 kodeky (Sega indexy bez ztraty, PS1 JPEG po castech: PSNR, stojici obraz, kvalita podle site, rozmery) s hlavickami B320" $(nula "$t") "$t"
  g++ -std=c++17 -O2 -pthread $TVP -o $P/test_hub test_hub.cpp && $P/test_hub $PNG320 2>&1 | cisty > $Z/VYSTUP_regrese_b305_hub.txt
  t=$(tail -1 $Z/VYSTUP_regrese_b305_hub.txt)
  vysl "B305 rozbocovac ve vlaknech (jadro 60 / s, zvuk, rychly a pomaly prohlizec) s hlavickami B320" $(nula "$t") "$t"
else
  radek "(B305 kodeky / hub preskoceny - PNG320 / PNG640 neni)"
fi
g++ -std=c++17 -O2 -pthread $TVP -o $P/test_pasy test_pasy.cpp && $P/test_pasy 2>&1 | cisty > $Z/VYSTUP_regrese_b305_pasy.txt
t=$(tail -1 $Z/VYSTUP_regrese_b305_pasy.txt)
vysl "B305 koder JPEG na vic jadrech (pasy 1 / 2 / 3, bajt po bajtu stejne) s B320 (barvy 4:4:4 volitelne)" $(nula "$t") "$t"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread $TVP -o $P/test_pasy_tsan test_pasy.cpp && $P/test_pasy_tsan > $P/pasy_tsan.txt 2>&1
n=$(grep -c 'WARNING: ThreadSanitizer' $P/pasy_tsan.txt)
vysl "B305 koder JPEG na vic jadrech pod ThreadSanitizerem" $([ "$n" = "0" ] && echo 1 || echo 0) "hlaseni ThreadSanitizer: $n, $(tail -1 $P/pasy_tsan.txt)"
cd $B/b313
g++ -std=c++17 -O2 -pthread $TVP -o $P/test_kodek_menu test_kodek_menu.cpp && $P/test_kodek_menu 2>&1 | cisty > $Z/VYSTUP_regrese_b313_kodek_menu.txt
t=$(tail -1 $Z/VYSTUP_regrese_b313_kodek_menu.txt)
vysl "B313 kodek hlavniho menu na TV (vic obdelniku, zjemneni za pohybu, glReadPixels) s hlavickami B320" $(nula "$t") "$t"
vrat test_overeni/b305 test_overeni/b313

# ---------- 1) testy B308 na kodu B320 ----------
cd $B/b308
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wall -Wno-unused-function $PS1 -o $P/libps1_b320.so ps1_harness.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 2> $P/preklad_harness.txt
vysl "preklad ps1_harness.cpp (B308) s nap_ps1_native.cpp B320, -Wl,--no-undefined" $([ -f $P/libps1_b320.so ] && echo 1 || echo 0) \
     "varovani: $(grep -c 'warning' $P/preklad_harness.txt), chyb: $(grep -c 'error' $P/preklad_harness.txt)"
rm -rf $P/denik
# B320: radek ZDRAVI ma navic ", z ovladace N" (herni ovladac) - test B308 ho cekal bez toho (git vrati)
python3 - <<'PY'
p = 'test_ps1_denik.py'
s = open(p, encoding='utf-8').read()
s = s.replace(r"z klavesnice TV \d+\) \| TV divaku 0", r"z klavesnice TV \d+, z ovladace \d+\) \| TV divaku 0")
open(p, 'w', encoding='utf-8').write(s)
PY
timeout 900 python3 test_ps1_denik.py $P/libps1_b320.so $P/denik > /dev/null 2>&1
cp VYSTUP_test_ps1_denik.txt $Z/VYSTUP_regrese_b308_test_ps1_denik.txt
t=$(tail -1 VYSTUP_test_ps1_denik.txt)
vysl "B308 denik PS1 + klavesnice z TV v C++ + zamek kolem retro_run (test_ps1_denik.py; ZDRAVI B320 navic ', z ovladace N')" $(nula "$t") "$t"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $PS1 -o $P/tsan_denik tsan_denik.cpp -l:libEGL.so.1 -l:libGLESv2.so.2
rm -rf $P/tsan_prac
TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 detect_deadlocks=0" timeout 900 $P/tsan_denik $P/tsan_prac > $P/tsan_denik.txt 2>&1
n=$(grep -c 'WARNING: ThreadSanitizer' $P/tsan_denik.txt)
vysl "B308 ThreadSanitizer: vlakno emulace, zvuk, klavesy z prohlizece, denik, SAVE / LOAD, dotyk (tsan_denik.cpp)" \
     $([ "$n" = "0" ] && grep -q "^stop" $P/tsan_denik.txt && echo 1 || echo 0) "hlaseni ThreadSanitizer: $n"
mkdir -p $P/jvmlib $P/jvmout; rm -rf $P/jvm_prac $P/jvm318 $P/jvm320
cp $P/libps1_b320.so $P/jvmlib/libnapps1core.so
# TestB318Mys pocita native metody (28 v B318) a TestB319Proud (29 v B319) - v B320 jich je 32
# (+ ps1OvladacKlavesa / ps1OvladacOsy / ps1OvladacPust - herni ovladac)
sed 's/nativnich == 28, "native metod ve tride: " + nativnich + " (23 z B316 + 4 mys B317 + ps1MysZrychleni B318)"/nativnich == 32, "native metod ve tride: " + nativnich + " (23 z B316 + 4 mys B317 + ps1MysZrychleni B318 + proudOtevri2Native B319 + 3 herni ovladac B320)"/' \
    $B/b318/jvm/TestB318Mys.java > $P/TestB318Mys.java
sed 's/nativnich == 29, "native metod ve tride: " + nativnich + " (28 z B318 + proudOtevri2Native B319)"/nativnich == 32, "native metod ve tride: " + nativnich + " (28 z B318 + proudOtevri2Native B319 + 3 herni ovladac B320)"/' \
    $B/b319/jvm/TestB319Proud.java > $P/TestB319Proud.java
javac -d $P/jvmout -sourcepath ../b306/jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java jvm/TestPodpisu.java \
    $P/TestB318Mys.java $P/TestB319Proud.java 2>&1 | grep -v JAVA_TOOL
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestPodpisu $P/jvm_prac 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$" | cisty > $Z/VYSTUP_regrese_b308_TestPodpisu.txt
t=$(tail -1 $Z/VYSTUP_regrese_b308_TestPodpisu.txt)
vysl "B308 JVM TestPodpisu (-Xcheck:jni): skutecna trida NativePs1CoreBridge B320" $(nula "$t") "$t"
timeout 900 python3 test_b308_klavesnice.py $P/libps1_b320.so $P/klavesnice > /dev/null 2>&1
cp VYSTUP_test_b308_klavesnice.txt $Z/VYSTUP_regrese_b308_test_b308_klavesnice.txt
t=$(tail -1 VYSTUP_test_b308_klavesnice.txt)
vysl "B308 klavesnice PC v TV prohlizeci -> PS1 (Chromium, stranka z MainActivity.java B320)" $(nula "$t") "$t"
LANG=cs_CZ.UTF-8 LANGUAGE=cs timeout 900 python3 test_b308_ps1_stranka.py > /dev/null 2>&1
cp VYSTUP_test_b308_ps1_stranka.txt $Z/VYSTUP_regrese_b308_test_b308_ps1_stranka.txt
t=$(tail -1 VYSTUP_test_b308_ps1_stranka.txt)
vysl "B308 stranka PS1: nahravani hry, banner, LOG s denikem (Chromium cesky)" $(nula "$t") "$t"
vrat test_overeni/b308

# ---------- 2) JVM: proud B319 (proudOtevri2) a mys B318 na kodu B320, symboly ----------
cd $Z
mkdir -p $Z/jvm
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestB319Proud $P/jvm320 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$\|^BUILD2S\|PIXFMT" | cisty > $Z/jvm/VYSTUP_TestB319Proud_na_B320.txt
t=$(tail -1 $Z/jvm/VYSTUP_TestB319Proud_na_B320.txt)
vysl "JVM TestB319Proud (-Xcheck:jni) na kodu B320: proudOtevri2Native(int) primo i pres obal, proud z bezici hry i staremu prohlizeci, log Hubu (B320: tolerance ditheringu)" \
     $(echo "$t" | grep -q " 0 chyb" && echo 1 || echo 0) "$t"
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestB318Mys $P/jvm318 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$\|^BUILD2S\|PIXFMT" | cisty > $Z/jvm/VYSTUP_TestB318Mys_na_B320.txt
t=$(tail -1 $Z/jvm/VYSTUP_TestB318Mys_na_B320.txt)
vysl "JVM TestB318Mys (-Xcheck:jni) na kodu B320: mys PS1 (B317 / B318) beze zmeny" $(echo "$t" | grep -q " 0 chyb" && echo 1 || echo 0) "$t"
{
  for m in $(grep -oE "native [][a-zA-Z]+ [a-zA-Z0-9_]+\(" $R/app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java | awk '{print $3}' | tr -d '('); do
    if nm -D --defined-only $P/libps1_b320.so | grep -q " Java_eu_atarihelp_emu10_NativePs1CoreBridge_${m}$"; then echo "OK     $m"; else echo "CHYBA  $m (v knihovne neni)"; fi
  done
} > $Z/jvm/VYSTUP_symboly.txt
vysl "kazda native metoda NativePs1CoreBridge ma v knihovne svou funkci (nm)" $([ "$(grep -c '^CHYBA' $Z/jvm/VYSTUP_symboly.txt)" = "0" ] && echo 1 || echo 0) \
     "$(grep -c '^OK' $Z/jvm/VYSTUP_symboly.txt) OK, $(grep -c '^CHYBA' $Z/jvm/VYSTUP_symboly.txt) CHYB"

# ---------- 3) testy TV PS1 B305 / B306: preklad + link ----------
cd $B/b305
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -I$R/app/src/main/cpp/vendor/stb -o $P/libps1_b305.so tv_ps1.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 > $P/preklad_b305.txt 2>&1
vysl "B305 tv_ps1.cpp (proud PS1 na TV) s nap_ps1_native.cpp B320: preklad + link" $([ -f $P/libps1_b305.so ] && echo 1 || echo 0) "$(tail -1 $P/preklad_b305.txt | cut -c1-120)"
cd $B/b306
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -I$R/app/src/main/cpp/vendor/stb -o $P/libps1_b306.so tv_ps1_6.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 > $P/preklad_b306.txt 2>&1
vysl "B306 tv_ps1_6.cpp s nap_ps1_native.cpp B320: preklad + link" $([ -f $P/libps1_b306.so ] && echo 1 || echo 0) "$(tail -1 $P/preklad_b306.txt | cut -c1-120)"

# ---------- 4) javac proti android.jar + podpisy JNI NativePs1CoreBridge ----------
if [ -n "$AJAR" ] && [ -f "$AJAR" ]; then
  rm -rf $P/javac_out && mkdir -p $P/javac_out
  (cd $R && timeout 900 $JV/bin/javac -proc:none --release 17 -cp "$AJAR" -Xmaxerrs 1000 -nowarn -d $P/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java > $P/javac.txt 2>&1)
  n=$(grep -c "error:" $P/javac.txt)
  {
    echo "B320 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    grep -v JAVA_TOOL $P/javac.txt
    echo "trid prelozeno: $(find $P/javac_out -name '*.class' | wc -l)"
    echo "VYSLEDEK: $n chyb prekladu"
  } > $Z/VYSTUP_javac_android.txt
  vysl "javac proti android.jar (cela appka)" $([ "$n" = "0" ] && echo 1 || echo 0) "$(tail -2 $Z/VYSTUP_javac_android.txt | tr '\n' ' ')"
  rm -rf $P/jnih && mkdir -p $P/jnih/h $P/jnih/out
  (cd $R && $JV/bin/javac -proc:none --release 17 -cp "$AJAR" -h $P/jnih/h -d $P/jnih/out \
      app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java app/src/main/java/eu/atarihelp/emu10/NativeSegaCoreBridge.java \
      app/src/main/java/eu/atarihelp/emu10/NativeAtariCoreBridge.java 2>&1 | grep -v JAVA_TOOL)
  python3 $Z/kontrola_jni.py $P/jnih/h > /dev/null
  t=$(tail -1 $Z/VYSTUP_kontrola_jni.txt)
  vysl "podpisy JNI: NativePs1CoreBridge / NativeSegaCoreBridge / NativeAtariCoreBridge (javac -h) = C++ (jmeno, navratovy typ, typy parametru)" \
       $(echo "$t" | grep -q " 0 CHYB" && echo 1 || echo 0) "$t"
else
  radek "(javac a podpisy JNI preskoceny - ANDROID_JAR neni)"
fi

# ---------- 5) stranka WEB TV: testy starsich verzi se strankou B320 ----------
(cd $B/b315 && timeout 1800 python3 test_b315_zvuk_tv.py > $P/b315.txt 2>&1)
cp $B/b315/VYSTUP_test_b315_zvuk_tv.txt $Z/VYSTUP_regrese_b315_zvuk_tv.txt
t=$(tail -1 $Z/VYSTUP_regrese_b315_zvuk_tv.txt)
vysl "B315 zvuk na TV / PC v dalsich hrach (menu -> Atari -> Sega -> PS1 -> MP3 ...), stranka B320" $(nula "$t") "$t"
vrat test_overeni/b315
(cd $B/b313 && timeout 600 python3 test_b313_tv_prepnuti.py > $P/b313.txt 2>&1)
cp $B/b313/VYSTUP_test_b313_tv_prepnuti.txt $Z/VYSTUP_regrese_b313_tv_prepnuti.txt
t=$(tail -1 $Z/VYSTUP_regrese_b313_tv_prepnuti.txt)
vysl "B313 TV / PC: prepnuti proudu po odchodu z menu (409 + X-Nap-Proud), stranka B320" $(nula "$t") "$t"
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
vysl "B302 klavesnice PC do Atari se strankou B320" $(nula "$t") "$t"
vrat test_overeni/b302
ATARI="-I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari"
(cd $B/b303 && g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $ATARI -o libtvproud.so tv_lib.cpp && \
  timeout 900 python3 test_b303_prohlizec.py > $P/b303.txt 2>&1; \
  cp VYSTUP_test_b303_prohlizec.txt $Z/VYSTUP_regrese_b303_atari_tv.txt; rm -f libtvproud.so)
t=$(tail -n 1 $Z/VYSTUP_regrese_b303_atari_tv.txt)
vysl "B303 Atari na TV (proud z C++, zvuk, Ctrl+V) se strankou B320" $(nula "$t") "$t"
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

# ---------- 6) SKUTECNE hlavni menu (nap_menu_gl.cpp, Mesa) - kodek menu z nap_tv_proud_jpeg.h B320 ----------
JNI="-I../b304/stub -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/tvproud"
(cd $B/b313 && g++ -std=c++17 -O2 -pthread -w $JNI -o $P/test_menu_host_b313 test_menu_host_b313.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 \
   && cd $P && timeout 600 ./test_menu_host_b313 2>&1 | grep -v "^karta \|^DEMO" | cisty > $Z/VYSTUP_regrese_b313_menu_host.txt)
t=$(tail -1 $Z/VYSTUP_regrese_b313_menu_host.txt)
tvj=$(grep -c "TV: stara cesta pres Javu (prohlizec bez proudu) 1280x720" $Z/VYSTUP_regrese_b313_menu_host.txt)
tvn=$(grep -c "TV: nikdo se nediva - TV obraz se nekresli" $Z/VYSTUP_regrese_b313_menu_host.txt)
vysl "B313 / B318 SKUTECNE menu na PC (Mesa): vse jako drive, radek MENU 'TV: nikdo se nediva' / 'TV: stara cesta pres Javu'" \
     $([ "$(nula "$t")" = "1" ] && [ "$tvj" -ge 1 ] && [ "$tvn" -ge 1 ] && echo 1 || echo 0) "$t; radku TV nikdo $tvn, stara cesta $tvj"
vrat test_overeni/b313

# ---------- 7) mys PS1 B318 na kodu B320: logika + SKUTECNE jadro PCSX (nap_ps1_native.cpp B320) ----------
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
  vysl "B318 SKUTECNE jadro PS1 (PCSX z appky, BIOS HLE) + nap_ps1_native.cpp B320: mys v hre, ktera kresli / cte kazdy 3. snimek" $(nula "$t") "$t"
else
  vysl "B318 SKUTECNE jadro PS1 + nap_ps1_native.cpp B320: preklad" 0 "$(grep -m1 error $P/preklad_integrace.txt | cut -c1-160)"
fi
vrat test_overeni/b318

# ---------- 8) testy B319 na kodu B320 (kodek B320 bez ditheringu = B319, stranka WEB TV B320) ----------
cd $B/b319
TV19="-I$R/app/src/main/cpp/tvproud -I$R/app/src/main/cpp/vendor/stb -I."
g++ -std=c++17 -O2 -pthread $TV19 -o $P/test_b319_kodek test_b319_kodek.cpp && $P/test_b319_kodek 2>&1 | cisty > $Z/VYSTUP_regrese_b319_kodek.txt
t=$(tail -1 $Z/VYSTUP_regrese_b319_kodek.txt)
vysl "B319 kodek (posun obrazu, polovicni rozliseni, 3D hra, vzory, rozmery) na kodeku B320" $(nula "$t") "$t"
g++ -std=c++17 -O2 -pthread $TV19 -o $P/test_b319_hub test_b319_hub.cpp && $P/test_b319_hub 2>&1 | cisty > $Z/VYSTUP_regrese_b319_hub.txt
t=$(tail -1 $Z/VYSTUP_regrese_b319_hub.txt)
vysl "B319 Hub ve vlaknech (jadro 60 / s, zvuk, prohlizec B319 a stary soubezne) na kodu B320" $(nula "$t") "$t"
g++ -std=c++17 -O1 -g -fsanitize=thread -pthread $TV19 -o $P/test_b319_hub_tsan test_b319_hub.cpp && \
  NAP_TSAN=1 TSAN_OPTIONS="halt_on_error=0" $P/test_b319_hub_tsan > $P/hub_tsan.txt 2>&1
n=$(grep -c 'WARNING: ThreadSanitizer' $P/hub_tsan.txt)
vysl "B319 Hub pod ThreadSanitizerem na kodu B320" $([ "$n" = "0" ] && echo 1 || echo 0) "hlaseni ThreadSanitizer: $n, $(grep -v '^WARNING\|^  \|^SUMMARY\|^==\|^$' $P/hub_tsan.txt | tail -1)"
g++ -std=c++17 -O2 -pthread $TV19 -o $P/gen_proud_b319 gen_proud_b319.cpp
for v in 3 0; do mkdir -p $P/proud_v$v && $P/gen_proud_b319 $P/proud_v$v $v > $P/gen_v$v.txt; done
timeout 900 python3 test_b319_prohlizec.py $P > $P/prohlizec19.txt 2>&1
cp VYSTUP_test_b319_prohlizec.txt $Z/VYSTUP_regrese_b319_prohlizec.txt
t=$(tail -1 $Z/VYSTUP_regrese_b319_prohlizec.txt)
vysl "B319 stranka WEB TV v Chromiu (proud B319: kopie + JPEG v polovicnim rozliseni, canvas = snimek hry; stara stranka B318), stranka B320" $(nula "$t") "$t"
python3 $Z/uprav_b319_tv_mys.py > /dev/null   # kontrola 4) stranky: B318 + B319 + 3 zmeny B320 (git vrati)
timeout 900 python3 test_b319_tv_mys.py > $P/tv_mys19.txt 2>&1
cp VYSTUP_test_b319_tv_mys.txt $Z/VYSTUP_regrese_b319_tv_mys.txt
t=$(tail -1 $Z/VYSTUP_regrese_b319_tv_mys.txt)
vysl "B319 mys pocitace na strance WEB TV (nejvys ~30 zprav / s, tlacitka hned, vse z B318), stranka B320 = B318 + B319 + jen 3 zmeny B320" $(nula "$t") "$t"
timeout 600 python3 test_b319_spojeni.py $P/spojeni > $P/spojeni19.txt 2>&1
cp VYSTUP_test_b319_spojeni.txt $Z/VYSTUP_regrese_b319_spojeni.txt
t=$(tail -1 $Z/VYSTUP_regrese_b319_spojeni.txt)
vysl "B319 mys z PC jednim spojenim: SKUTECNY kod serveru z MainActivity B320 v JVM + Chromium" $(nula "$t") "$t"
vrat test_overeni/b319

radek "VYSLEDEK: $ok OK, $ch CHYBA"
tail -1 $OUT
