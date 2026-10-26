#!/bin/bash
# B316: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b316)
#   CARTRIDGE (.car) PRO ATARI 130XE - Rene: "slo by do atari emu pridat i priponu *.car - tedy aby
#   tyto hry sly v nasem emu spustit?" (poslal Wonderboy_VBXE.car - typ 42, Atarimax 1 MB, hra pro VBXE).
#   Slot a 67 typu cartridge v C++ (nap_atari_cart.h), jadro (nap_atari_machine.h: okna $8000-$BFFF,
#   $D5xx, TRIG3), JNI devLoadCarNative + devCtl cart / cart_eject, Java: rozpoznani .car (hlavicka
#   CART, i v ZIP a z webu), XEX/MOBIL -> VYSUNOUT CARTRIDGE, navod.
# Spoustet odkudkoli: PRAC=... ANDROID_JAR=... CAR=.../Wonderboy_VBXE.car bash test_overeni/b316/vse.sh
# Potreba: g++ / gcc, curl (zdrojaky Atari800 z GitHubu - do repozitare se nedavaji, GPL), Python 3,
# JAVA_HOME = JDK, android.jar API 34 ($ANDROID_JAR), Chromium + Playwright pro Python (navod B314).
# CAR = Reneho Wonderboy_VBXE.car (do repozitare se nedava) - bez nej se skutecna hra preskoci.
set -u
PRAC=${PRAC:-/tmp/nap_b316}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
CAR=${CAR:-}
R=$(cd "$(dirname "$0")/../.." && pwd)
A=$R/app/src/main/cpp/atari
mkdir -p $PRAC
cd $R/test_overeni/b316

echo "== 1) cartridge proti emulatoru Atari800 (src/cartridge.c, commit 98cc2dc): tabulka typu, nacteni .car, prepinani bank"
ATARI800=https://raw.githubusercontent.com/atari800/atari800/98cc2dc749867c9c7317a0271ba062a6c797f886/src
mkdir -p $PRAC/atari800
for f in cartridge.c cartridge.h cartridge_info.c cartridge_info.h; do
  [ -f $PRAC/atari800/$f ] || curl -sSfL -o $PRAC/atari800/$f $ATARI800/$f || echo "CHYBA stazeni $f"
done
gcc -std=gnu99 -O2 -w -I$PRAC/atari800 -Iref_stub -c $PRAC/atari800/cartridge.c -o $PRAC/cartridge.o && \
gcc -std=gnu99 -O2 -w -I$PRAC/atari800 -Iref_stub -c $PRAC/atari800/cartridge_info.c -o $PRAC/cartridge_info.o && \
gcc -std=gnu99 -O2 -Iref_stub -c ref_stub/ref_stub.c -o $PRAC/ref_stub.o && \
g++ -std=c++17 -O2 -Wall -I$A -I$PRAC/atari800 -Iref_stub -o $PRAC/test_b316_cart_diff test_b316_cart_diff.cpp \
    $PRAC/cartridge.o $PRAC/cartridge_info.o $PRAC/ref_stub.o || echo CHYBA_PREKLAD_DIFF
if [ -n "$CAR" ]; then $PRAC/test_b316_cart_diff --prac $PRAC --car "$CAR" > VYSTUP_test_b316_cart_diff.txt 2>&1
else $PRAC/test_b316_cart_diff --prac $PRAC > VYSTUP_test_b316_cart_diff.txt 2>&1; fi
sed -i "s#$PRAC#PRAC#g; s#${CAR:-@@}#Wonderboy_VBXE.car#g" VYSTUP_test_b316_cart_diff.txt
tail -1 VYSTUP_test_b316_cart_diff.txt

echo "== 2) cartridge v jadre 130XE (OS ji spusti, ANTIC, RD4/RD5, TRIG3, RAM pod ni, Bounty Bob, XEGS, RESET/POWER, skutecna hra)"
g++ -std=c++17 -O2 -Wall -I$A -o $PRAC/test_b316_core test_b316_core.cpp || echo CHYBA_PREKLAD_CORE
mkdir -p $PRAC/snimky
$PRAC/test_b316_core ${CAR:+"$CAR"} ${CAR:+$PRAC/snimky/wb} > VYSTUP_test_b316_core.txt 2>&1
sed -i "s#${CAR:-@@}#Wonderboy_VBXE.car#g; s#[0-9.]* s ([0-9]* snimku/s#(cas behu na PC) (rychlost#" VYSTUP_test_b316_core.txt
tail -1 VYSTUP_test_b316_core.txt

echo "== 3) skutecny nap_atari_native.cpp s vlakny (JNI jako Java): .car, chyby, POWER, VYSUNOUT, jine programy"
g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JV/include -I$JV/include/linux -I$A -I../b292 \
    -o $PRAC/test_b316_jni_host test_b316_jni_host.cpp 2>/dev/null || echo CHYBA_PREKLAD_JNI
timeout 900 $PRAC/test_b316_jni_host ${CAR:+"$CAR"} > VYSTUP_test_b316_jni_host.txt 2>&1
sed -i "s#${CAR:-@@}#Wonderboy_VBXE.car#g" VYSTUP_test_b316_jni_host.txt
tail -1 VYSTUP_test_b316_jni_host.txt

echo "== 4) Java proti SKUTECNEMU Androidu (android.jar API 34) + JNI podpisy"
if [ -n "${ANDROID_JAR:-}" ] && [ -f "$ANDROID_JAR" ]; then
  rm -rf $PRAC/javac_out && mkdir -p $PRAC/javac_out
  { echo "B316 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    n=$(cd $R && timeout 900 $JV/bin/javac -proc:none --release 17 -cp "$ANDROID_JAR" -Xmaxerrs 1000 -nowarn -d $PRAC/javac_out -h $PRAC/jni_h \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -v JAVA_TOOL | tee $PRAC/javac.txt | grep -c "error:")
    cat $PRAC/javac.txt
    echo "trid prelozeno: $(find $PRAC/javac_out -name '*.class' | wc -l)"
    echo "VYSLEDEK: $n chyb prekladu"; } > VYSTUP_javac_android.txt
  tail -1 VYSTUP_javac_android.txt
  # hlavicka JNI z Javy (javac -h) vlozena pred nap_atari_native.cpp: kazda nativni metoda Atari musi mit
  # v C++ funkci se STEJNYMI typy (jinak "conflicting declaration"), a naopak
  { echo "B316 - JNI: hlavicka z NativeAtariCoreBridge.java (javac -h) proti nap_atari_native.cpp"
    H=$PRAC/jni_h/eu_atarihelp_emu10_NativeAtariCoreBridge.h
    if g++ -std=c++17 -fsyntax-only -pthread -I../b291/stub -I$JV/include -I$JV/include/linux -I$A -include $H \
         $A/nap_atari_native.cpp > $PRAC/jni_kontrola.txt 2>&1; then echo "OK     preklad s hlavickou z Javy bez chyby"; c=0
    else echo "CHYBA  preklad s hlavickou z Javy:"; head -20 $PRAC/jni_kontrola.txt; c=1; fi
    jn=$(grep -o "Java_eu_atarihelp_emu10_NativeAtariCoreBridge_[A-Za-z0-9_]*" $H | sort -u)
    cn=$(grep -o "Java_eu_atarihelp_emu10_NativeAtariCoreBridge_[A-Za-z0-9_]*" $A/nap_atari_native.cpp | sort -u)
    chybi=$(comm -23 <(echo "$jn") <(echo "$cn") | wc -l)
    echo "nativnich metod v Jave $(echo "$jn" | wc -l), v C++ $(echo "$cn" | wc -l), v C++ chybi $chybi"
    echo "$jn" | grep -q devLoadCarNative && echo "OK     devLoadCarNative(byte[], String) -> String je v Jave i v C++" || c=$((c+1))
    [ "$chybi" = "0" ] || c=$((c+1))
    echo "VYSLEDEK: $c CHYBA"; } > VYSTUP_jni_podpisy.txt
  tail -1 VYSTUP_jni_podpisy.txt
else
  echo "   (preskoceno - ANDROID_JAR neni nastaveny)"
fi

echo "== 5) navod v appce: co presne se zmenilo proti B315 (jen XEX / MOBIL a NET / HRY)"
python3 kontrola_navodu_b316.py > /dev/null; tail -1 VYSTUP_kontrola_navodu_b316.txt

echo "== 6) regrese: jadro bez cartridge = B315 (test_core vsechny rezimy + snimky her), testy s vlakny B291/B292, navod"
bash regrese.sh "$PRAC" "$JV" > /dev/null 2>&1; tail -1 VYSTUP_regrese.txt
