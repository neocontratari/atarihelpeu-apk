BUILD2NL_SEGA_CLOWNMD_HARD_POWER_RELOAD_STAGE25

CIL:
Opravit zpomalovani po RESETu a po vymene ROM. BUILD2NK uz mel funkcni ovladac, ale reset pouze menil wrapper/iframe a stare ClownMDEmu runtime smycky zrejme zustavaly v Android WebView. BUILD2NL dela tvrdsi POWER CYCLE cele SEGA stranky.

CO TESTOVAT:
1. Rozbal ZIP pres koren projektu.
2. Spust APK/Nox.
3. Otevri Sega modul.
4. Klikni HRY / SBIRKA / CARTRIDGE SLOT / prostredni vlozenou cartridge.
5. Vyber Sonic .gen.
6. Ocekavani: stranka se kratce restartuje, pak sama nacte Sonic pres lokalni wrapper.
7. Over ovladac:
   - DPAD jen chuze/smery, nesmi skakat.
   - B = hlavni akce/skok.
   - C = sekundarni akce/skok/potvrzeni dle hry.
   - A = special/menu dle hry.
   - START funguje.
8. Klikni RESET vpravo u cartridge/power panelu.
9. Ocekavani: celá Sega stranka udela power-cycle a nacte stejnou ROM znovu; dalsi beh nesmi byt brutalne zpomaleny.
10. Vyber jinou ROM pres CARTRIDGE SLOT/HRY/SBIRKA.
11. Ocekavani: znovu power-cycle a nova ROM bez zpomaleni.
12. Zkus opakovane RESET 3x po sobe. Nemelo by dochazet ke kumulativnimu zpomalovani.
13. Zkus landscape.
14. Klikni ULOZENE a posli log.

LOG MARKERY:
- BUILD2NL ROM SELECTED
- BUILD2NL ROM READ OK
- BUILD2NL HARDBOOT_STORE indexedDB OK nebo sessionStorage OK
- BUILD2NL PAGE_POWER_RELOAD
- BUILD2NL HARDBOOT_LOAD ... OK
- BUILD2NL HARDBOOT AUTOBOOT
- BUILD2NL ROM_POST_TO_LOCAL_WRAPPER
- WRAPPER MODULE_POSTRUN
- BUILD2NL HARD POWER RESET current ROM

KDYZ SELZE:
Poslat screenshot + TXT log. Hlavne hledej, jestli selhalo HARDBOOT_STORE, HARDBOOT_LOAD, nebo az WRAPPER boot.
