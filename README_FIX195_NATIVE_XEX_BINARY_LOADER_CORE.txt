FIX195_NATIVE_XEX_BINARY_LOADER_CORE

Smer:
- Zadna dalsi pomocna testovaci tlacitka ani latani jedne hry.
- Standardni XEX nacitani je ted Native-style XEX Binary Loader Core pod kapotou.
- Loader tolerantne overi $FF $FF hlavicku, projde bloky start/end, nasype data primo do emulovane RAM, sleduje RUNAD $02E0/$02E1 a INITAD $02E2/$02E3.
- BASIC/ROM direct spousteni hotovych XEX her zustava vypnute.
- ROMky zustavaji pro REAL OS/BASIC testy, ne pro direct pousteni her.
- CPU po loadu dostane clean/cold stav a PC jde na RUNAD/ENTRY z loaderu.
- Snapshot pise NATIVE XEX LOADER FIX195.

Dulezite:
- V aktualnim repozitari je jadro stale Android WebView/JS overlay, ne samostatny C++ modul. Implementace je proto native-style binary loader v existujicim core, aby slo rovnou buildnout APK pres stavajici GitHub Actions.

Test:
1. Nacist TEST DONKEY KONG XEX.
2. Nacist TEST SUPER COBRA XEX.
3. Nacist Montezuma / Arkanoid / Pitstop II pres bezny vyber XEX nebo vestavene testy.
4. U kazdeho poslat screenshot + snapshot a zkontrolovat radek NATIVE XEX LOADER FIX195.
5. REAL OS BASIC / G7 TUNNEL / GTIA BARS jen smoke, ze se nerozbily potvrzene veci.
