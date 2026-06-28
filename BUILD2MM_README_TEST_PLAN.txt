AtariHelp.eu EMU-10 BUILD2MM_SEGA_AHSAVE_DOWNLOAD_LOG_SAFE

ZAKLAD:
- BUILD2ML / Sega modul
- hlavni N&P VISION menu beze zmen
- EMULATOR -> Atari/VBXE EMU2
- spodni leve ATARI -> SEGA modul
- ROM picker beze zmen
- Sonic/ROM neni v ZIPu ani APK

OPRAVA:
- BUILD2ML zkousel ulozit log pres window.Android / browser download, ale aktualni MainActivity pouziva AHSAVE bridge.
- BUILD2MM vola primo AHSAVE.save(filename,text).
- To uklada TXT do Downloads/AtariHelp pres existujici MainActivity AHSave.

V LOGU / MONITORU HLEDEJ:
AtariHelp.eu EMU-10 BUILD2MM_SEGA_AHSAVE_DOWNLOAD_LOG_SAFE pripraven
SEGA LOG DOWNLOAD BUILD2MM: AHSAVE.save -> DOWNLOADS_OK:Downloads/AtariHelp/...

TEST:
1) SEGA modul
2) nacti .gen
3) klikni ULOZENE
4) v monitoru musi pribyt ULOZENI LOGU s cestou
5) v PC/Nox hledej: Downloads/AtariHelp/AtariHelp_SEGA_LOG_YYYYMMDD_HHMMSS.txt

KODY JSOU ZMENENE jen v emu_sega/index.html.
