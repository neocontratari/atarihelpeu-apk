AtariHelp.eu EMU-10 BUILD2MI_RETRO_MAIN_FOOTER_SEGA_EXACT_SAFE

UCEL:
- hlavni N&P VISION obrazovka zustava stejna
- velke tlacitko EMULATOR vede na EMU2 / Atari 130XE VBXE: emu_vbxe/index.html
- spodni leve ATARI / 8-BIT LEGACY vede na SEGA MEGA DRIVE modul: emu_sega/index.html
- PREHRAVAC / ATARIHELP.EU / HELP / Rene Frank / Retro Power zachovano

ZMENENE SOUBORY:
- app/src/main/assets/index.html
- app/src/main/assets/emu_sega/index.html
- app/src/main/assets/emu_sega/sega_megadrive_screen.png

NEZMENENO:
- Atari/VBXE jadro
- CLOAD/CSAVE
- prehravac
- web/help routing mimo popisek
- ROMy ani Sonic nejsou vlozene
- zadny fake emulator

TEST:
1) Spust appku.
2) Velke EMULATOR -> musi otevrit Atari 130XE/VBXE EMU2.
3) MENU z Atari zpet na hlavni N&P VISION.
4) Spodni leve ATARI / 8-BIT LEGACY -> musi otevrit SEGA MEGA DRIVE obrazovku 1:1.
5) MENU v SEGA obrazovce -> musi vratit na hlavni N&P VISION.
6) PREHRAVAC / ATARIHELP.EU / HELP musi zustat funkcni.
