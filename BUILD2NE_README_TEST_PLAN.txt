BUILD2NE_SEGA_CLOWNMD_IN_APP_FRAME_KEYS_STAGE18 - TEST PLAN

1) Rozbal ZIP pres koren projektu.
2) Spust APK/Nox.
3) Otevri Sega modul.
4) Klikni HRY nebo CARTRIDGE SLOT.
5) Cekany vysledek: original Sega skin zustane, oficialni ClownMDEmu se nacte uvnitr horniho monitoru.
6) V monitoru pouzij menu Software a vyber Sonic ROM.
7) Cekany vysledek: grafika Sonic bezi v nasi Sega obrazovce, ne na cele cizi strance.
8) Zkus nase tlacitka: D-PAD, A, B, C, START.
9) Klikni ULOZENE a posli log.

DULEZITE:
- Pokud hra bezi, ale nase tlacitka nereaguji, neni to graficky problem. Log pravdepodobne ukaze blokaci syntetickych klaves do cross-origin iframe.
- Dalsi krok potom musi byt native WebView KeyEvent bridge nebo lokalni ClownMDEmu build, ne dalsi kresleni.
