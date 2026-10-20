#!/bin/bash
# B312: vsechny testy najednou (vystupy VYSTUP_* sem do test_overeni/b312)
#   MP3 PREHRAVAC: RECORD (mixtape) a FAVORITES, DZUBOX RECORD (M4A / WebM s delkou),
#   SERVIS (ODEMKNOUT DALSI / ZAMKNOUT VSE pod servisnim kodem), ZAMKY SEGA / PS1 i pro hry z webu,
#   smazane nepouzite obrazky, navod.
# Spoustet odkudkoli: PRAC=... ANDROID_JAR=... [NAP_SERVIS_KOD=...] bash test_overeni/b312/vse.sh
# Potreba (viz JAK_SPUSTIT.txt): JAVA_HOME = JDK, Python 3, Chromium + Playwright pro Python,
# ffmpeg / ffprobe (s libmp3lame), android.jar API 34 ($ANDROID_JAR - jen pro preklad Javy).
# NAP_SERVIS_KOD = skutecny servisni kod (zna ho jen Rene) - kdyz je nastaveny, JVM test overi
# i otisk zabudovany v appce a bod 10 hleda, jestli kod neni v repozitari (kod se nikam nevypisuje).
set -u
PRAC=${PRAC:-/tmp/nap_b312}
JV=${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk-amd64}
R=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p $PRAC
cd $R/test_overeni/b312
cisty() { sed "s#$PRAC#PRAC#g; s#$R#REPO#g"; }

echo "== 0) C++ se v B312 nemenilo (testy C++ menu, Atari, Segy, PS1 z B311 a starsich plati dal)"
n=$(cd $R && git diff --name-only 21c3aad -- app/src/main/cpp | wc -l)
echo "   souboru v app/src/main/cpp zmenenych od B311 (21c3aad): $n" | tee VYSTUP_cpp_beze_zmeny.txt

echo "== 1) JVM: NapServis (PBKDF2 proti Pythonu, kod, pokusy, 10 min) a NapPravidla (zamky, hudba, ZPET)"
rm -rf $PRAC/jvm && mkdir -p $PRAC/jvm
python3 pbkdf2_vektory.py $PRAC/vektory.txt > /dev/null
$JV/bin/javac --release 17 -d $PRAC/jvm $R/app/src/main/java/eu/atarihelp/emu10/NapServis.java \
  $R/app/src/main/java/eu/atarihelp/emu10/NapPravidla.java TestB312Java.java 2>&1 | grep -v JAVA_TOOL
$JV/bin/java -Dstdout.encoding=UTF-8 -cp $PRAC/jvm eu.atarihelp.emu10.TestB312Java $PRAC/vektory.txt 2>&1 | grep -v JAVA_TOOL > VYSTUP_test_b312_java.txt
tail -1 VYSTUP_test_b312_java.txt

echo "== 2) MainActivity: kazda cesta ke hre Segy / PS1 se nejdriv zepta na zamek, servis hlida Java"
python3 kontrola_zamku.py > /dev/null; tail -1 VYSTUP_kontrola_zamku.txt

echo "== 3) MP3 PREHRAVAC v Chromiu: knihovna, FAVORITES, RECORD (mixtape), ZPET, WebM, chyba zapisu, PC"
timeout 900 python3 test_b312_prehravac.py $PRAC/prehravac > /dev/null 2>&1; tail -1 VYSTUP_test_b312_prehravac.txt

echo "== 4) DZUBOX v Chromiu: RECORD M4A / WebM s delkou, chyba zapisu, PC, odchod behem nahravani"
timeout 600 python3 test_b312_dj.py $PRAC/dj > /dev/null 2>&1; tail -1 VYSTUP_test_b312_dj.txt

echo "== 5) OPTIONS v Chromiu: servisni tlacitka schovana, kod, pokusy, 10 minut"
timeout 600 python3 test_b312_servis.py $PRAC/servis > /dev/null 2>&1; tail -1 VYSTUP_test_b312_servis.txt

echo "== 6) navod v Chromiu (B311 + B312) a nazvy tlacitek v navodu proti appce"
timeout 600 python3 test_b312_navod.py $PRAC/navod > /dev/null 2>&1; tail -1 VYSTUP_test_b312_navod.txt
python3 kontrola_navodu.py > /dev/null; tail -1 VYSTUP_kontrola_navodu.txt

echo "== 7) mosty JavaScript -> Java a odkazy na soubory (i smazane obrazky)"
python3 kontrola_mostu.py > VYSTUP_kontrola_mostu.txt 2>&1; tail -1 VYSTUP_kontrola_mostu.txt
python3 kontrola_odkazu.py > /dev/null; tail -1 VYSTUP_kontrola_odkazu.txt

echo "== 8) Java appky prelozena proti SKUTECNEMU Androidu (android.jar API 34, Java 17 jako v build.gradle)"
if [ -n "${ANDROID_JAR:-}" ] && [ -f "$ANDROID_JAR" ]; then
  rm -rf $PRAC/javac_out && mkdir -p $PRAC/javac_out
  { echo "B312 - javac --release 17 -cp android.jar (API 34) vsechny zdrojaky appky (eu.atarihelp.emu10 + nap)"
    (cd $R && timeout 900 $JV/bin/javac -proc:none --release 17 -cp "$ANDROID_JAR" -Xmaxerrs 1000 -nowarn -d $PRAC/javac_out \
      app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -v JAVA_TOOL)
    echo "trid prelozeno: $(find $PRAC/javac_out -name '*.class' | wc -l)"
    n=$(cd $R && $JV/bin/javac -proc:none --release 17 -cp "$ANDROID_JAR" -nowarn -d $PRAC/javac_out app/src/main/java/eu/atarihelp/emu10/*.java app/src/main/java/eu/atarihelp/nap/*.java 2>&1 | grep -c "error:")
    echo "VYSLEDEK: $n chyb prekladu"; } > VYSTUP_javac_android.txt
  tail -1 VYSTUP_javac_android.txt
else
  echo "   (preskoceno - ANDROID_JAR neni nastaveny)"
fi

echo "== 9) regrese: hlavni menu B311 v Chromiu (zaloha v HTML, C++ menu, OPTIONS, WEB TV) se strankou z B312"
(cd ../b311 && cp VYSTUP_test_b311_menu.txt $PRAC/b311_menu_puvodni.txt && \
  timeout 600 python3 test_b311_menu.py $PRAC/menu > /dev/null 2>&1; \
  cp VYSTUP_test_b311_menu.txt ../b312/VYSTUP_b311_menu_se_strankou_b312.txt; \
  cp $PRAC/b311_menu_puvodni.txt VYSTUP_test_b311_menu.txt)
tail -1 VYSTUP_b311_menu_se_strankou_b312.txt
echo "   (A11 = 10 kroku testu B311 na strance Atari 130XE - v B312 jsou tam nove kroky B312; ty kontroluje bod 5, test_b312_servis.py 9)"

echo "== 10) servisni kod NENI v repozitari (soubory ani historie) - jen s NAP_SERVIS_KOD"
if [ -n "${NAP_SERVIS_KOD:-}" ]; then
  { echo "B312 - hledani servisniho kodu v repozitari (kod se nevypisuje)"
    a=$(cd $R && git grep -c -F "$NAP_SERVIS_KOD" -- . 2>/dev/null | wc -l)
    b=$(cd $R && grep -rlF --exclude-dir=.git "$NAP_SERVIS_KOD" . 2>/dev/null | wc -l)
    c=$(cd $R && timeout 1200 git log --all --oneline -S"$NAP_SERVIS_KOD" 2>/dev/null | wc -l)
    echo "souboru v gitu s kodem: $a, souboru na disku s kodem: $b, commitu s kodem v historii: $c"
    echo "VYSLEDEK: $((a + b + c)) nalezu (ma byt 0)"; } > VYSTUP_kod_neni_v_repu.txt
  tail -1 VYSTUP_kod_neni_v_repu.txt
else
  echo "   (preskoceno - NAP_SERVIS_KOD neni nastaveny)"
fi
