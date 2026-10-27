#!/bin/bash
# B317: REGRESE PS1 - s vypnutou mysi se PS1 chova presne jako B316; JVM / JNI / javac
#   1) testy B308 na kodu B317 (SKUTECNY nap_ps1_native.cpp, misto jadra "hra" z ps1_harness.cpp):
#      denik PS1, klavesnice z TV v C++, zamek kolem retro_run, ThreadSanitizer, JVM -Xcheck:jni
#      (TestPodpisu B308), klavesnice v prohlizeci TV (Chromium), stranka PS1 - nahravani hry
#   2) JVM: nove native metody mysi (jvm/TestB317Mys.java) se skutecnou tridou NativePs1CoreBridge,
#      -Xcheck:jni; kazda native metoda tridy ma v knihovne svou funkci
#   3) testy TV PS1 B305 / B306 (tv_ps1.cpp, tv_ps1_6.cpp - nap_ps1_native.cpp s jinymi nahradami): preklad + link
#   4) javac proti android.jar (API 34) - cela appka; podpisy JNI NativePs1CoreBridge (javac -h) = C++
# Vysledky: VYSTUP_regrese.txt (souhrn) + VYSTUP_regrese_b308_* (cele vystupy testu B308 na kodu B317);
# puvodni VYSTUP_* v b308 se vrati z gitu.
#   bash regrese.sh PRAC INC JAVA_HOME ANDROID_JAR
set -u
PRAC=$1; INC=$2; JV=$3; AJAR=${4:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
B=$R/test_overeni
Z=$B/b317
OUT=$Z/VYSTUP_regrese.txt
P=$PRAC/reg317
mkdir -p $P
export EGL_PLATFORM=surfaceless
PS1="-DANDROID -I$B/b305/stub -I$B/b291/stub -I$INC -I$JV/include -I$JV/include/linux"
ok=0; ch=0
radek() { echo "$1" >> $OUT; }
vysl() {   # vysl "popis" ok(0/1) "detail"
  if [ "$2" = "1" ]; then ok=$((ok+1)); radek "OK     $1  -- $3"; else ch=$((ch+1)); radek "CHYBA  $1  -- $3"; fi
}
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
echo "B317 - regrese PS1 (mys vypnuta = B316), JVM, JNI, javac" > $OUT

# ---------- 1) testy B308 na kodu B317 ----------
cd $B/b308
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wall -Wno-unused-function $PS1 -o $P/libps1_b317.so ps1_harness.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 2> $P/preklad_harness.txt
vysl "preklad ps1_harness.cpp (B308) s nap_ps1_native.cpp B317, -Wl,--no-undefined" $([ -f $P/libps1_b317.so ] && echo 1 || echo 0) \
     "varovani: $(grep -c 'warning' $P/preklad_harness.txt), chyb: $(grep -c 'error' $P/preklad_harness.txt)"
rm -rf $P/denik
timeout 900 python3 test_ps1_denik.py $P/libps1_b317.so $P/denik > /dev/null 2>&1
cp VYSTUP_test_ps1_denik.txt $Z/VYSTUP_regrese_b308_test_ps1_denik.txt
t=$(tail -1 VYSTUP_test_ps1_denik.txt)
vysl "B308 denik PS1 + klavesnice z TV v C++ + zamek kolem retro_run (test_ps1_denik.py)" $(echo "$t" | grep -q " 0 CHYB\| 0 chyb\|0 CHYBA" && echo 1 || echo 0) "$t"

g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -Wno-unused-function $PS1 -o $P/tsan_denik tsan_denik.cpp -l:libEGL.so.1 -l:libGLESv2.so.2
rm -rf $P/tsan_prac
TSAN_OPTIONS="halt_on_error=0 report_signal_unsafe=0 detect_deadlocks=0" timeout 900 $P/tsan_denik $P/tsan_prac > $P/tsan_denik.txt 2>&1
n=$(grep -c 'WARNING: ThreadSanitizer' $P/tsan_denik.txt)
vysl "B308 ThreadSanitizer: vlakno emulace, zvuk, klavesy z prohlizece, denik, SAVE / LOAD, dotyk (tsan_denik.cpp)" $([ "$n" = "0" ] && grep -q "^stop" $P/tsan_denik.txt && echo 1 || echo 0) \
     "hlaseni ThreadSanitizer: $n; $(grep '^stop' $P/tsan_denik.txt | head -1 | cut -c1-100)"

mkdir -p $P/jvmlib $P/jvmout; rm -rf $P/jvm_prac $P/jvm317
cp $P/libps1_b317.so $P/jvmlib/libnapps1core.so
javac -d $P/jvmout -sourcepath ../b306/jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java jvm/TestPodpisu.java \
    $Z/jvm/TestB317Mys.java 2>&1 | grep -v JAVA_TOOL
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestPodpisu $P/jvm_prac 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$" | cisty > $Z/VYSTUP_regrese_b308_TestPodpisu.txt
t=$(tail -1 $Z/VYSTUP_regrese_b308_TestPodpisu.txt)
vysl "B308 JVM TestPodpisu (-Xcheck:jni): skutecna trida NativePs1CoreBridge B317" $(echo "$t" | grep -q " 0 chyb\| 0 CHYB" && echo 1 || echo 0) "$t"

timeout 900 python3 test_b308_klavesnice.py $P/libps1_b317.so $P/klavesnice > /dev/null 2>&1
cp VYSTUP_test_b308_klavesnice.txt $Z/VYSTUP_regrese_b308_test_b308_klavesnice.txt
t=$(tail -1 VYSTUP_test_b308_klavesnice.txt)
vysl "B308 klavesnice PC v TV prohlizeci -> PS1 (Chromium, stranka z MainActivity.java)" $(echo "$t" | grep -q " 0 CHYB\| 0 chyb\|0 CHYBA" && echo 1 || echo 0) "$t"

# Test B308 je starsi nez jazyk (B314): stranka bez volby jazyka bere jazyk prohlizece - v anglickem
# Chromiu je anglicky a test (hleda ceske texty) je nenajde. Proto Chromium cesky (jako telefon Reneho);
# pro srovnani i anglicky Chromium se strankou B316 a B317 (stejny vysledek = nic noveho).
LANG=cs_CZ.UTF-8 LANGUAGE=cs timeout 900 python3 test_b308_ps1_stranka.py > /dev/null 2>&1
cp VYSTUP_test_b308_ps1_stranka.txt $Z/VYSTUP_regrese_b308_test_b308_ps1_stranka.txt
t=$(tail -1 VYSTUP_test_b308_ps1_stranka.txt)
vysl "B308 stranka PS1: nahravani hry podle skutecneho stavu, banner, LOG s denikem (Chromium cesky, stranka B317)" $(echo "$t" | grep -q " 0 CHYB\| 0 chyb\|0 CHYBA" && echo 1 || echo 0) "$t"
timeout 900 python3 test_b308_ps1_stranka.py > /dev/null 2>&1
t317=$(tail -1 VYSTUP_test_b308_ps1_stranka.txt)
rm -rf $P/b316str && mkdir -p $P/b316str && (cd $R && git archive d7c85b9 app/src/main/assets test_overeni/b308) | tar -x -C $P/b316str
(cd $P/b316str/test_overeni/b308 && timeout 900 python3 test_b308_ps1_stranka.py > /dev/null 2>&1)
t316=$(tail -1 $P/b316str/test_overeni/b308/VYSTUP_test_b308_ps1_stranka.txt)
vysl "B308 stranka PS1 v anglickem Chromiu: B317 stejne jako B316 (ceske texty test nenajde od B314 - nic noveho)" $([ "$t317" = "$t316" ] && echo 1 || echo 0) "B316: $t316 | B317: $t317"
# puvodni vystupy B308 zpet
(cd $R && git checkout -- test_overeni/b308/VYSTUP_test_ps1_denik.txt test_overeni/b308/UKAZKA_denik_ps1.txt \
    test_overeni/b308/VYSTUP_test_b308_klavesnice.txt test_overeni/b308/VYSTUP_test_b308_ps1_stranka.txt 2>/dev/null)

# ---------- 2) JVM: nove metody mysi ----------
timeout 120 java -Xcheck:jni -Djava.library.path=$P/jvmlib -cp $P/jvmout TestB317Mys $P/jvm317 2>/dev/null \
    | grep -v "^20[0-9][0-9]-\|^CESTA\|^\[\|^$" | cisty > $Z/jvm/VYSTUP_TestB317Mys.txt
t=$(tail -1 $Z/jvm/VYSTUP_TestB317Mys.txt)
vysl "JVM TestB317Mys (-Xcheck:jni): ps1MysNastav / ps1MysStav / ps1MysDotyk / ps1PcMys primo i pres obaly MainActivity" \
     $(echo "$t" | grep -q " 0 chyb" && echo 1 || echo 0) "$t"
{
  for m in $(grep -oE "native [][a-zA-Z]+ [a-zA-Z0-9_]+\(" $R/app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java | awk '{print $3}' | tr -d '('); do
    if nm -D --defined-only $P/libps1_b317.so | grep -q " Java_eu_atarihelp_emu10_NativePs1CoreBridge_${m}$"; then echo "OK     $m"; else echo "CHYBA  $m (v knihovne neni)"; fi
  done
} > $Z/jvm/VYSTUP_symboly.txt
vysl "kazda native metoda NativePs1CoreBridge ma v knihovne svou funkci (nm)" $([ "$(grep -c '^CHYBA' $Z/jvm/VYSTUP_symboly.txt)" = "0" ] && echo 1 || echo 0) \
     "$(grep -c '^OK' $Z/jvm/VYSTUP_symboly.txt) OK, $(grep -c '^CHYBA' $Z/jvm/VYSTUP_symboly.txt) CHYB"

# ---------- 3) testy TV PS1 B305 / B306: preklad + link ----------
cd $B/b305
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -I$R/app/src/main/cpp/vendor/stb -o $P/libps1_b305.so tv_ps1.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 > $P/preklad_b305.txt 2>&1
vysl "B305 tv_ps1.cpp (proud PS1 na TV) s nap_ps1_native.cpp B317: preklad + link" $([ -f $P/libps1_b305.so ] && echo 1 || echo 0) "$(tail -1 $P/preklad_b305.txt | cut -c1-120)"
cd $B/b306
g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $PS1 -I$R/app/src/main/cpp/vendor/stb -o $P/libps1_b306.so tv_ps1_6.cpp \
    -Wl,--no-undefined -l:libEGL.so.1 -l:libGLESv2.so.2 > $P/preklad_b306.txt 2>&1
vysl "B306 tv_ps1_6.cpp s nap_ps1_native.cpp B317: preklad + link" $([ -f $P/libps1_b306.so ] && echo 1 || echo 0) "$(tail -1 $P/preklad_b306.txt | cut -c1-120)"

# ---------- 4) javac proti android.jar + podpisy JNI NativePs1CoreBridge ----------
if [ -n "$AJAR" ] && [ -f "$AJAR" ]; then
  rm -rf $P/javac_out && mkdir -p $P/javac_out
  (cd $R && timeout 900 $JV/bin/javac -proc:none --release 17 -cp "$AJAR" -Xmaxerrs 1000 -nowarn -d $P/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java > $P/javac.txt 2>&1)
  n=$(grep -c "error:" $P/javac.txt)
  {
    echo "B317 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
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

radek "VYSLEDEK: $ok OK, $ch CHYBA"
tail -1 $OUT
