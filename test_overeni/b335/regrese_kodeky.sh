#!/bin/bash
# B335: testy kodeku / Hubu starsich verzi s hlavickami z pracovniho stromu R -> vystupy do O
# Pouziti: bash regrese_kodeky.sh REPO VYSTUPY PRAC PNG320 PNG640   (REPO = strom B335 nebo cisty B334 -
# git worktree add --detach PRAC/wt334 32f83db); PNG320 / PNG640 = snimky 3D sceny (../b305/JAK_SPUSTIT.txt krok 0)
# (kazdy test bezi v kopii sve slozky v P, repozitar se nemeni)
set -u
R=$1; O=$2; P=$3; PNG320=$4; PNG640=$5
mkdir -p $O $P
T=$R/test_overeni
TV=$R/app/src/main/cpp/tvproud
STB=$R/app/src/main/cpp/vendor/stb
kopie() { rm -rf "$P/$1"; mkdir -p "$P/$1"; cp -r "$T/$1/." "$P/$1/"; }
for d in b305 b306 b313 b319 b320 b333; do kopie $d; done
beh() {
  local d=$1 jm=$2 vys=$3; shift 3
  (cd $P/$d && "$@" > $O/$jm.log 2>&1)
  cp $O/$jm.log $O/$jm.txt   # testy pisi na stdout (VYSTUP_* v repozitari = presmerovany stdout)
  echo "$jm: $(grep -a 'VYSLEDEK' $O/$jm.txt | tail -1)"
}
cc() { g++ -std=c++17 -O2 -pthread -I$TV -I$STB "$@" 2>&1 | grep -E "error" | head -5; }
(cd $P/b305 && cc -I. -o test_kodeky test_kodeky.cpp && cc -o test_pasy test_pasy.cpp && cc -I. -o test_hub test_hub.cpp)
beh b305 b305_kodeky VYSTUP_test_kodeky.txt ./test_kodeky $PNG320 $PNG640
beh b305 b305_pasy VYSTUP_test_pasy.txt ./test_pasy
beh b305 b305_hub VYSTUP_test_hub.txt ./test_hub $PNG320
(cd $P/b306 && cc -o test_hub6 test_hub6.cpp)
beh b306 b306_hub6 VYSTUP_test_hub6.txt ./test_hub6
(cd $P/b313 && cc -I. -o test_kodek_menu test_kodek_menu.cpp)
beh b313 b313_kodek_menu VYSTUP_test_kodek_menu.txt ./test_kodek_menu
(cd $P/b319 && cc -I. -o test_b319_kodek test_b319_kodek.cpp && cc -I. -o test_b319_hub test_b319_hub.cpp)
beh b319 b319_kodek VYSTUP_test_b319_kodek.txt ./test_b319_kodek
beh b319 b319_hub VYSTUP_test_b319_hub.txt ./test_b319_hub
(cd $P/b320 && cc -I../b319 -I. -o test_b320_kodek test_b320_kodek.cpp)
beh b320 b320_kodek VYSTUP_test_b320_kodek.txt ./test_b320_kodek
mkdir -p $P/b333/prac
(cd $P/b333 && cc -I../b313 -o test_stranka test_stranka.cpp)
beh b333 b333_stranka VYSTUP_test_stranka.txt ./test_stranka $P/b333/prac
echo HOTOVO
