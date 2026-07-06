BUILD2SA5AS_STAGE193_TEKKEN_MULTIBIN_ENOSPC_CLEANSCREEN

TRI OPRAVY (MainActivity.java + emu_ps1/index.html; C++ NEDOTCENO):

1) TEKKEN 3 FIX (tvuj screenshot to odhalil):
   Zip ma 3 .bin tracky BEZ .cue - extraktor bral PRVNI nalezeny, coz byl
   Track 3 = 28MB AUDIO stopa, ne hra => zaseknuti na 92 %.
   Ted se bez .cue bere NEJVETSI .bin (datovy track; "track 1" ma prednost).
   Poctive: hra pojede, ale CD hudba z audio tracku hrat nebude (na to je
   potreba .cue verze hry). Marker: PS1_REMOTE_ZIP_PRIMARY ... size=...

2) ENOSPC BALIK (z predchoziho SA5AR, tady na zaklade STAGE191):
   auto-uklid starych her pred stazenim, mazani ZIPu po rozbaleni,
   kontrola volneho mista se srozumitelnou hlaskou,
   SMAZAT CACHE funguje i pri bezici hre (napred hru zastavi).

3) OBRAZ PO SMAZANI CACHE: obrazovka hry ted po smazani zmizi
   (drive zustal posledni snimek - vypadalo to jako nesmazano; data
   smazana byla, jen obrazek visel).

SONY LOGO + ZNELKA - vysvetleni (neni to zamer ani nas bug):
Volba loga je v jadre uz zapnuta. Znelka nehraje, protoze jadro ted bezi
na sve VESTAVENE nahrade BIOSu (HLE) - hry jedou, ale autenticky boot
s logem umi jen PRAVY BIOS (tvuj scph1001). Dohledani, proc si jadro
tvuj ulozeny BIOS nebere, je samostatny krok - dam ho do plánu spolu
s dynarecem (zvuk/rychlost). Nic nemusis delat.

TEST (S8): 1) Tekken 3 z netu - MUSI dojet pres 92 % a nabehnout
2) SMAZAT CACHE pri bezici hre - obraz zmizi 3) dve hry po sobe - zadny ENOSPC.
Zaklad = tvuj STAGE191; kdyby workflow cerveny, screenshot staci.
