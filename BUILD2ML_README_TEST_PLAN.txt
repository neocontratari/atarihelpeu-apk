AtariHelp.eu EMU-10 BUILD2ML_SEGA_AUTO_DOWNLOAD_LOG_SAFE

ZAKLAD:
- BUILD2MK
- hlavni N&P VISION menu beze zmen
- EMULATOR -> Atari/VBXE EMU2 beze zmen
- spodni leve ATARI -> SEGA modul beze zmen
- Atari/VBXE/CLOAD/CSAVE beze zmen
- Sonic/ROM neni v ZIP ani APK

ZMENA:
- V SEGA modulu tlacitko ULOZENE = LOG
- LOG se po kliknuti nejen zobrazi v monitoru, ale zkusi se automaticky ulozit jako TXT do Downloads/Stazeni:
  AtariHelp_SEGA_LOG_YYYYMMDD_HHMMSS.txt
- Pouzita je robustni kaskada:
  Android save bridge pokud existuje -> Blob download -> data URL download -> fallback jen monitor

LOG MARKERY:
- AtariHelp.eu EMU-10 BUILD2ML_SEGA_AUTO_DOWNLOAD_LOG_SAFE pripraven
- SEGA MONITOR CLEAN BUILD2ML
- SEGA LOG DOWNLOAD BUILD2ML
- SEGA ROM PICKER BUILD2ML
- SEGA CORE NOT FAKE BUILD2ML

TEST:
1) Hlavni menu -> spodni leve ATARI -> SEGA
2) HRY/SBIRKA/CARTRIDGE -> vyber .gen/.bin/.md/.smd
3) ULOZENE -> ma se zobrazit LOG a automaticky ulozit TXT do Downloads/Stazeni
4) Poslat zpatky ulozeny TXT nebo screenshot, pokud ulozeni selze

KODY JSOU ZMENENE pouze v app/src/main/assets/emu_sega/index.html.
