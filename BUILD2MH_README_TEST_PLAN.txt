AtariHelp.eu EMU-10 BUILD2MH_RETRO_SEGA_EXACT_SCREEN_SLOT_SAFE

CIL:
- Zakladni N&P VISION obrazovku NEMENIT.
- Velke tlacitko EMULATOR ma dal otevirat nase EMU2 / Atari 130XE VBXE.
- Spodni leve tlacitko ATARI / puvodni EMU2 legacy slot ma otevrit novy SEGA MEGA DRIVE modul.
- PREHRAVAC, ATARIHELP.EU, HELP a ostatni funkce zachovat.

CO JE V ZIPU:
- app/src/main/assets/emu_sega/index.html
- app/src/main/assets/emu_sega/sega_megadrive_screen.png
- redirect aliasy:
  emu, emu1, emu_1, emulator -> emu_vbxe/index.html
  emu2, emu_2, atari, atari_8bit, atari8, atari8bit, legacy, legacy_atari, atari_legacy -> emu_sega/index.html
- root index.html se NEPRIBALUJE, aby se hlavni grafika nerozbila.
- Atari/VBXE core se NEPRIBALUJE a NEMENI.

KODY JSOU ZMENENE jen v router/slot vrstve a novem SEGA assetu.
Zadny Sonic ROM. Zadny Mega Drive core. Zadna fake emulace.

TEST:
1. Aplikuj ZIP pres GitHub Desktop.
2. Sestav APK.
3. Spust appku.
4. Zakladni N&P VISION obrazovka musi zustat stejna.
5. Klikni velke EMULATOR:
   - musi otevrit Atari 130XE / VBXE EMU2.
6. Vrat se pres MENU na hlavni obrazovku.
7. Klikni spodni leve ATARI / 8-BIT LEGACY:
   - musi otevrit presne dodanou SEGA MEGA DRIVE obrazovku.
8. V SEGA obrazovce klikni MENU dole vpravo:
   - ma se vratit na hlavni menu.
9. Otestuj PREHRAVAC, ATARIHELP.EU, HELP:
   - musi zustat zachovane.

KDYZ SPODNI LEVE ATARI PORAD OTEVRE ATARI/VBXE:
- hlavni menu nemiri pres emu2/atari alias, ale primo na emu_vbxe/index.html nebo ma inline onclick v root index.html.
- potom je potreba upravit pouze ten konkretni onclick/hitbox v aktualnim app/src/main/assets/index.html.
