#!/bin/bash
# B314: regrese - testy starsich verzi na kodu B314 (jazyk sahl na menu, OPTIONS, stranky, Javu,
# intro, pristroj Atari a stranku WEB TV). Vysledky sem (VYSTUP_regrese*.txt), puvodni VYSTUP_*
# starsich verzi se vrati z gitu (git checkout), aby zustaly, jak byly ve sve verzi.
#   bash regrese.sh PRAC INC JAVA_HOME      (GLSLANG, ANDROID_JAR z prostredi jako v ../b313/vse.sh)
set -u
PRAC=$1; INC=$2; JV=$3
R=$(cd "$(dirname "$0")/../.." && pwd)
B=$R/test_overeni
OUT=$B/b314/VYSTUP_regrese.txt
mkdir -p $PRAC/reg314
export EGL_PLATFORM=surfaceless
ok=0; ch=0
radek() { echo "$1" >> $OUT; }
vysl() {   # vysl "popis" "vysledek" "vzor pro OK"
  if echo "$2" | grep -q "$3"; then ok=$((ok+1)); radek "OK     $1  -- $2"; else ch=$((ch+1)); radek "CHYBA  $1  -- $2"; fi
}
vrat() { (cd $R && git checkout -- "$@" 2>/dev/null); }
echo "B314 - regrese (testy B291, B312, B313 a jejich regrese na kodu B314)" > $OUT

# 1) B313 vse.sh: TV prepnuti, kodek menu (+TSan), SKUTECNE menu pres JNI (+TSan), menu na TV v Chromiu,
#    konzole, shadery, javac + JNI, a jeho regrese.sh (B305 kodeky / pasy / rozbocovac + TSan, B306,
#    B309 intro z Javy bit po bitu, cele demo, B311 karusel + snimky menu + nazvy v navodu, B302, B303)
PRAC=$PRAC/b313 INC=$INC JAVA_HOME=$JV bash $B/b313/vse.sh > $PRAC/reg314/b313_vse.txt 2>&1
for f in VYSTUP_kontrola_zalohy VYSTUP_test_b313_tv_prepnuti VYSTUP_test_kodek_menu VYSTUP_test_kodek_menu_tsan \
         VYSTUP_test_menu_host_b313 VYSTUP_test_b313_tv_menu VYSTUP_test_konzole_anim VYSTUP_konzole_nahled \
         VYSTUP_kontrola_shaderu VYSTUP_javac_android VYSTUP_regrese; do
  cp $B/b313/$f.txt $B/b314/VYSTUP_regrese_b313_${f#VYSTUP_}.txt 2>/dev/null
done
cp $B/b313/VYSTUP_tsan_menu_host_b313.txt $B/b314/VYSTUP_regrese_b313_tsan_menu_host_b313.txt 2>/dev/null
cp $B/b313/VYSTUP_kontrola_jni.txt $B/b314/VYSTUP_regrese_b313_kontrola_jni.txt 2>/dev/null
cp $PRAC/reg314/b313_vse.txt $B/b314/VYSTUP_regrese_b313_vse_konzole.txt
vysl "B313 postup po nove instalaci (zaloha Androidu vypnuta)" "$(tail -1 $B/b313/VYSTUP_kontrola_zalohy.txt)" " 0 CHYB\|0 CHYBA\|, 0 "
vysl "B313 TV / PC prepnuti proudu po odchodu z menu (Chromium, stranka z MainActivity)" "$(tail -1 $B/b313/VYSTUP_test_b313_tv_prepnuti.txt)" " 0 CHYB"
vysl "B313 kodek JPEG pro menu na TV" "$(tail -1 $B/b313/VYSTUP_test_kodek_menu.txt)" " 0 CHYB\| 0 chyb"
vysl "B313 kodek JPEG pro menu pod ThreadSanitizerem" "$(tail -1 $B/b313/VYSTUP_test_kodek_menu_tsan.txt)" "hlaseni ThreadSanitizeru: 0"
vysl "B313 SKUTECNE menu pres JNI (bez jazyka - jako B313)" "$(tail -1 $B/b313/VYSTUP_test_menu_host_b313.txt)" " 0 CHYBA"
vysl "B313 SKUTECNE menu pres JNI pod ThreadSanitizerem" "$(tail -1 $B/b313/VYSTUP_tsan_menu_host_b313.txt)" "0 zavodu v nasem kodu"
vysl "B313 menu na TV / PC v Chromiu (ostrost, kvalita, klavesnice)" "$(tail -1 $B/b313/VYSTUP_test_b313_tv_menu.txt)" " 0 CHYB"
vysl "B313 konzole N&P 16 / 32: animace" "$(tail -1 $B/b313/VYSTUP_test_konzole_anim.txt)" " 0 CHYB\| 0 chyb\|0 CHYBA"
vysl "B313 shadery GLSL ES 1.00" "$(tail -1 $B/b313/VYSTUP_kontrola_shaderu.txt)" " 0 chyb\| 0 CHYB\|: 0"
vysl "B313 Java proti android.jar" "$(tail -1 $B/b313/VYSTUP_javac_android.txt)" "VYSLEDEK: 0 chyb"
vysl "B313 podpisy JNI menu (Java = C++)" "$(grep VYSLEDEK $B/b313/VYSTUP_kontrola_jni.txt | tail -1)" "0 nesouhlasi"
vysl "B313 regrese (B305, B306, B309, B307, B311, B302, B303)" "$(tail -1 $B/b313/VYSTUP_regrese.txt)" ", 0 CHYBA"
vrat test_overeni/b313 test_overeni/b311 test_overeni/b302 test_overeni/b303

# 2) B312 vse.sh: JVM (NapServis, NapPravidla), zamky v MainActivity, MP3 prehravac, DZUBOX, OPTIONS servis,
#    navod, nazvy tlacitek, mosty JS -> Java, odkazy, javac, menu B311 se strankou
PRAC=$PRAC/b312 ANDROID_JAR=${ANDROID_JAR:-} JAVA_HOME=$JV bash $B/b312/vse.sh > $PRAC/reg314/b312_vse.txt 2>&1
for f in test_b312_java kontrola_zamku test_b312_prehravac test_b312_dj test_b312_servis test_b312_navod kontrola_navodu \
         kontrola_mostu kontrola_odkazu javac_android b311_menu_se_strankou_b312; do
  cp $B/b312/VYSTUP_$f.txt $B/b314/VYSTUP_regrese_b312_$f.txt 2>/dev/null
done
cp $PRAC/reg314/b312_vse.txt $B/b314/VYSTUP_regrese_b312_vse_konzole.txt
vysl "B312 JVM: NapServis a NapPravidla (cesky beze zmeny)" "$(tail -1 $B/b312/VYSTUP_test_b312_java.txt)" " 0 CHYB\| 0 chyb\|0 CHYBA"
vysl "B312 kazda cesta ke hre Segy / PS1 se pta na zamek" "$(tail -1 $B/b312/VYSTUP_kontrola_zamku.txt)" " 0 CHYB\| 0 chyb\|0 CHYBA"
vysl "B312 MP3 prehravac (knihovna, FAVORITES, RECORD) se strankou B314" "$(tail -1 $B/b312/VYSTUP_test_b312_prehravac.txt)" " 0 CHYB"
vysl "B312 DZUBOX RECORD" "$(tail -1 $B/b312/VYSTUP_test_b312_dj.txt)" " 0 CHYB"
vysl "B312 OPTIONS: servisni kod, ODEMKNOUT DALSI / ZAMKNOUT VSE (s prepinacem jazyka)" "$(tail -1 $B/b312/VYSTUP_test_b312_servis.txt)" " 0 CHYB"
vysl "B312 navod v Chromiu" "$(tail -1 $B/b312/VYSTUP_test_b312_navod.txt)" " 0 CHYB"
vysl "B312 nazvy tlacitek v ceskem navodu proti appce" "$(tail -1 $B/b312/VYSTUP_kontrola_navodu.txt)" " 0 CHYBA\|, 0 CHYB"
vysl "B312 mosty JS -> Java (vcetne AHJAZYK)" "$(tail -1 $B/b312/VYSTUP_kontrola_mostu.txt)" " 0 CHYB\|0 chyb\|: 0"
vysl "B312 odkazy na soubory (en.html, jazyk/*.js, obrazky _en)" "$(tail -1 $B/b312/VYSTUP_kontrola_odkazu.txt)" " 0 CHYB\|0 chyb\|: 0"
vysl "B312 hlavni menu B311 v Chromiu se strankou B314" "$(tail -1 $B/b312/VYSTUP_b311_menu_se_strankou_b312.txt)" "CHYB"
vrat test_overeni/b312 test_overeni/b311

# 3) B291: SKUTECNY nap_atari_native.cpp (pristroj, dotyk, kazeta, CSAVE) - pristroj ma v B314 jazyk
(cd $B/b291 && g++ -std=c++17 -O2 -pthread -Istub -I$JV/include -I$JV/include/linux -I../../app/src/main/cpp/atari \
   -o $PRAC/reg314/test_b291_jni_host test_b291_jni_host.cpp > $PRAC/reg314/b291_preklad.txt 2>&1 && \
   timeout 900 $PRAC/reg314/test_b291_jni_host > $PRAC/reg314/b291_jni.txt 2>&1)
cp $PRAC/reg314/b291_jni.txt $B/b314/VYSTUP_regrese_b291_jni_host.txt 2>/dev/null
vysl "B291 SKUTECNY nap_atari_native.cpp: pristroj, dotyk, kazeta (s jazykem B314)" "$(grep -i 'VYSLEDEK\|celkem' $PRAC/reg314/b291_jni.txt | tail -1)" " 0 CHYB\| 0 chyb\|0 FAIL\|FAIL 0\|: 0 "
vrat test_overeni/b291

radek "VYSLEDEK: $ok OK, $ch CHYBA"
