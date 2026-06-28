AtariHelp.eu EMU-10 BUILD2MK_SEGA_MONITOR_CLEAN_LOG_SAFE

ZAKLAD:
- navazuje na BUILD2MJ
- hlavni N&P VISION menu zustava stejne
- EMULATOR -> Atari 130XE/VBXE EMU2 zustava
- spodni leve ATARI -> SEGA modul zustava
- Atari/VBXE jadro, CLOAD/CSAVE, prehravac, web, help beze zmen
- Sonic ROM neni v ZIPu ani APK

ZMENY:
1) SEGA monitor vycisten
- podkres Sonic screenshotu v monitoru je prekryty cernym SEGA monitorem
- monitor je pripraven pro budouci realny video canvas

2) LOG tlacitko
- tlacitko ULOZENE v Sega obrazovce funguje jako LOG
- zobrazi SEGA log v monitoru
- pokusi se zkopirovat log do schranky

3) ROM picker zustava
- SBIRKA / HRY / cartridge slot otevre .gen/.bin/.md/.smd
- vypise realny Mega Drive header, checksum, region, CRC32

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2MK_SEGA_MONITOR_CLEAN_LOG_SAFE pripraven
SEGA MONITOR CLEAN BUILD2MK
SEGA LOG BUTTON BUILD2MK
SEGA ROM PICKER BUILD2MK
SEGA CORE NOT FAKE BUILD2MK

TEST:
1) Hlavni menu -> spodni leve ATARI -> SEGA
2) Monitor nesmi ukazovat staticky Sonic podkres, ma byt cerny SEGA READY monitor
3) ULOZENE -> zobrazi LOG
4) SBIRKA/HRY/cartridge -> vyber Sonic .gen lokalne
5) Monitor ukaze ROM CHECK OK + title/region/checksum

STATUS:
- zatim to nespousti hru, pouze overuje ROM a pripravuje realny core
