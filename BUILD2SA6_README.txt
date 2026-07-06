BUILD2SA6_STAGE194_PS1_DYNAREC_ARM64_BIOS_AUDIT

HLAVNI VEC: DYNAREC (ari64) PRO ARM64 - rychly preklad PS1 procesoru.
Stejny typ zazraku jako -O3 u Segy: interpreter byl brzda zvuku i rychlosti.
- CMakeLists: dynarec zdroje + arm64 assembly presne dle jni/Android.mk jadra,
  DRC_DISABLE pryc, NDRC_THREAD zapnut (dynarec ma vlastni vlakno).
- Volba pcsx_rearmed_drc=enabled uz v kodu byla - ted se realne pouzije.
- OVERENO: cely napps1core s dynarecem vc. assembly zkompilovan a slinkovan
  ARM64 krizovym prekladem s --no-undefined. Napoprvé cisto.
- OCEKAVANI: vyrazne plynulejsi hry a CISTSI ZVUK (chrceni bylo hladoveni
  pomaleho jadra). Kdyby nejaka hra s dynarecem zlobila, napis KTERA -
  jadro ma per-hru workaroundy.

BONUS: BIOS AUDIT - v PS1_BOOT_OK radku je nove sysdirFiles=[...]:
presny vypis, co jadro vidi v systemove slozce. Kdyz tam neuvidime
scph1001.bin, mame pricinu chybejiciho SONY loga/znelky (jadro jede na
vestavene nahrade). Posli mi ten radek - podle nej dodelame pravy BIOS boot.

ZMENENE SOUBORY: CMakeLists.txt + nap_ps1_native.cpp (jen BIOS vypis).
MainActivity/stranky NEDOTCENY (STAGE193 opravy zustavaji).

TEST (S8): 1) Actions zeleny 2) hra z netu: MUSI byt znatelne plynulejsi
a zvuk cistsi 3) posli PS1_BOOT_OK radek se sysdirFiles + rekni pocity
ze zvuku (cisty/lepsi/stejny) 4) Tekken/Crash/MoH prubezne - kdyby nektera
hra s dynarecem spadla nebo zlobila, jmeno hry staci.
