BUILD2SA10_STAGE198_BIOS_FRIENDLY_BANNER (obsahuje SA7-SA9)

OPRAVA: instalace BIOSu pres LOAD GAME uz NEUKAZUJE cerveny "chyba" banner.
Nove se ukaze normalni zprava: PS1_BIOS_INSTALLED "BIOS NAINSTALOVAN (N) -
tohle byl BIOS, ne hra. Ted spust HRU a SONY logo pojede."
(BIOS se instaloval spravne uz v SA9 - jen zprava byla oblecena jako chyba.)

DALSI PLAN (odsouhlasit poradi):
1) MEMORY CARD per hra - ulozeni pozic podle nazvu hry, hra si je vzdy najde.
   (retro_get_memory SAVE_RAM -> soubor <hra>.srm, nacteni pri bootu,
    ulozeni prubezne + pri stopu). Nejvyssi priorita - bez toho se neda hrat
   dlouha hra.
2) INTERNI ULOZISTE vs SD KARTA - volba, kam se instaluji hry.
3) JOYSTICK ODEZVA - napred zmerime (muze to byt nahledovy obraz 10fps,
   ne vstup) - pasivni audit jako u Segy, pak cileny fix.
4) EXTERNI GAMEPAD (bluetooth/USB) - jde to! Android KeyEvents -> jadro.
5) TV ZRCADLENI - tvuj S8 to umi UZ TED bez kodu: Smart View (zrcadleni
   obrazovky na TV) - vyzkousej; vlastni "cisty" TV vystup (jen hra bez UI)
   je mozny pozdejsi nadstandard.
TEST SA10: LOAD GAME na BIOS ZIP -> NORMALNI zprava (ne chyba) -> hra -> logo.
