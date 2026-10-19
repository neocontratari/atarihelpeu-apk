#!/bin/bash
# B310: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b310)
# Spoustet odkudkoli: bash test_overeni/b310/vse.sh
# Co je potreba (viz JAK_SPUSTIT.txt): g++, JAVA_HOME = JDK (javac, hlavicky JNI), git,
# Python 3 + Chromium + Playwright pro Python. Zadny Android SDK (Java bez Androidu viz 6).
set -u
PRAC=${PRAC:-/tmp/nap_b310}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b310
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }
ATARI="-I../b291/stub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari"

echo "== 1) POSTUP HRACE v C++: skutecny nap_atari_native.cpp (pristroj), psani prstem / z PC / Ctrl+V"
g++ -std=c++17 -O2 -pthread -Wno-unused-function $ATARI -o $PRAC/test_b310_postup test_b310_postup.cpp || echo CHYBA_PREKLAD
timeout 900 $PRAC/test_b310_postup > $PRAC/postup.txt 2>&1
{ echo "B310 - postup hrace (odemceni Segy) v C++: skutecny nap_atari_native.cpp, pristroj Atari 130XE"
  echo "(radky B310 / B299 jsou z logu pristroje, ostatni vypisuje test)"; echo
  grep "^OK\|^CHYBA\|^[0-9])\|^   \|^VYSLEDEK\|^B310 \|^B299 KLAVESY\|^B303 VLOZENI" $PRAC/postup.txt | cisty; } > VYSTUP_test_b310_postup.txt
tail -1 VYSTUP_test_b310_postup.txt

echo "== 2) JVM: skutecna trida NapPostup (davky z C++, Sega od 50 radku, davka pod 5 se nepocita)"
rm -rf $PRAC/jvmout && mkdir -p $PRAC/jvmout
javac -d $PRAC/jvmout -sourcepath jvm/stub ../../app/src/main/java/eu/atarihelp/emu10/NapPostup.java jvm/TestPostup.java 2>&1 | grep -v JAVA_TOOL
java -cp $PRAC/jvmout TestPostup 2>&1 | grep -v JAVA_TOOL > jvm/VYSTUP_TestPostup.txt; tail -1 jvm/VYSTUP_TestPostup.txt

echo "== 3) hlavni nabidka v Chromiu: ATARI 130XE EMULATOR -> C++ Atari, HELP = napoveda, TV POKUS, zamcena Sega"
timeout 300 python3 test_b310_menu.py $PRAC > $PRAC/menu.txt 2>&1; tail -1 VYSTUP_test_b310_menu.txt

echo "== 4) mosty JavaScript -> Java: kazde volani AH*.metoda() ze stranek ma metodu v Jave"
python3 kontrola_mostu.py > VYSTUP_kontrola_mostu.txt 2>&1; tail -1 VYSTUP_kontrola_mostu.txt

echo "== 5) odkazy: nic nevede na smazane soubory stareho Atari"
python3 kontrola_odkazu.py > VYSTUP_kontrola_odkazu.txt 2>&1; tail -1 VYSTUP_kontrola_odkazu.txt

echo "== 6) Java bez Androidu: javac B309 (git 7b40d21) proti B310 - zadna nova vlastni chyba"
rm -rf $PRAC/jc && mkdir -p $PRAC/jc
for v in old new; do mkdir -p $PRAC/jc/$v/eu/atarihelp/emu10 $PRAC/jc/$v/eu/atarihelp/nap; done
for d in emu10 nap; do
  for f in $(cd $R/app/src/main/java/eu/atarihelp/$d && ls *.java); do
    git -C $R show 7b40d21:app/src/main/java/eu/atarihelp/$d/$f > $PRAC/jc/old/eu/atarihelp/$d/$f 2>/dev/null
    cp $R/app/src/main/java/eu/atarihelp/$d/$f $PRAC/jc/new/eu/atarihelp/$d/$f
  done
done
for v in old new; do
  (cd $PRAC/jc/$v && timeout 600 javac -proc:none -Xmaxerrs 100000 -Xmaxwarns 0 -nowarn -d $PRAC/jc/out_$v -sourcepath . eu/atarihelp/emu10/*.java eu/atarihelp/nap/*.java > $PRAC/jc/err_$v.txt 2>&1)
done
python3 javac_porovnej.py $PRAC/jc/err_old.txt $PRAC/jc/err_new.txt > VYSTUP_javac_rozdil.txt; tail -1 VYSTUP_javac_rozdil.txt

echo "== 7) regrese Atari 130XE (C++): jadro (klavesnice PC, vkladani, proud na TV) a cely pristroj (B292)"
g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o $PRAC/test_core ../b292/test_core.cpp || echo CHYBA_PREKLAD_CORE
{ echo "B310 - testy jadra Atari z B292 (../b292/test_core.cpp) s kodem B310"
  for t in pc-klavesnice vlozeni tv-proud; do (cd ../b292 && timeout 600 $PRAC/test_core $t 2>&1 | tail -1); done; } > VYSTUP_b292_core_se_b310.txt
cat VYSTUP_b292_core_se_b310.txt | tail -3
g++ -std=c++17 -O2 -pthread -Wno-unused-function $ATARI -o $PRAC/test_b292_jni_host ../b292/test_b292_jni_host.cpp || echo CHYBA_PREKLAD_JNI
{ echo "B310 - cely pristroj Atari 130XE (../b292/test_b292_jni_host.cpp: klavesnice, CSAVE/CLOAD, ATR, VBXE,"
  echo "PC klavesnice, Ctrl+V, proud na TV, ovladani na sirku) se SKUTECNYM nap_atari_native.cpp z B310"; echo
  (cd ../b292 && timeout 1200 $PRAC/test_b292_jni_host 2>&1) | grep "^OK\|^CHYBA\|^VYSLEDEK\|preskocen" | cisty; } > VYSTUP_b292_jni_host_se_b310.txt
tail -1 VYSTUP_b292_jni_host_se_b310.txt

echo "== 8) regrese TV / PC v Chromiu se strankou z MainActivity B310: klavesnice PC (B302) a Atari na TV + Ctrl+V (B303)"
(cd ../b302 && cp VYSTUP_test_b302_prohlizec.txt $PRAC/b302_puvodni.txt && \
  g++ -std=c++17 -O2 -shared -fPIC -I../../app/src/main/cpp/atari -o libpcklav.so pc_klav_lib.cpp && \
  timeout 900 python3 test_b302_prohlizec.py > $PRAC/b302_log.txt 2>&1; \
  cp VYSTUP_test_b302_prohlizec.txt ../b310/VYSTUP_b302_klavesnice_pc_se_strankou_b310.txt; \
  cp $PRAC/b302_puvodni.txt VYSTUP_test_b302_prohlizec.txt; rm -f libpcklav.so)
tail -1 VYSTUP_b302_klavesnice_pc_se_strankou_b310.txt
(cd ../b303 && cp VYSTUP_test_b303_prohlizec.txt $PRAC/b303_puvodni.txt && \
  g++ -std=c++17 -O2 -shared -fPIC -pthread -Wno-unused-function $ATARI -o libtvproud.so tv_lib.cpp && \
  timeout 900 python3 test_b303_prohlizec.py > $PRAC/b303_log.txt 2>&1; \
  cp VYSTUP_test_b303_prohlizec.txt ../b310/VYSTUP_b303_atari_tv_se_strankou_b310.txt; \
  cp $PRAC/b303_puvodni.txt VYSTUP_test_b303_prohlizec.txt; rm -f libtvproud.so)
tail -1 VYSTUP_b303_atari_tv_se_strankou_b310.txt
