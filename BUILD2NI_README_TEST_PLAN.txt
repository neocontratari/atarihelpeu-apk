BUILD2NI_SEGA_CLOWNMD_LOCAL_WRAPPER_BLOB_BOOT_STAGE22

TEST:
1) Rozbal ZIP pres koren projektu.
2) Spust APK/Nox.
3) Otevri SEGA modul.
4) Klikni HRY / SBIRKA / CARTRIDGE SLOT.
5) Vyber Sonic .gen.
6) Cekej 10-20 sekund.
7) Pokud se zobrazi hra, zkus START, DPAD, A/B/C.
8) Zkus otoceni na landscape.
9) Klikni ULOZENE a posli LOG + screenshot.

CEKANE LOG MARKERY:
- BUILD2NI ROM SELECTED
- BUILD2NI ROM READ OK
- BUILD2NI ROM_POST_TO_LOCAL_WRAPPER
- WRAPPER WRAPPER_READY
- WRAPPER BOOTING_ROM
- WRAPPER SCRIPT_APPEND https://sonicresearch.org/clownacy/clownmdemu.js
- WRAPPER CLOWN_SCRIPT_LOADED nebo presna SCRIPT/WASM/RUNTIME chyba

POCTIVE:
- Zadny fake Sonic.
- Zadna ROM v APK/ZIP.
- Zadna nova grafika; puvodni Sega skin zachovan.
