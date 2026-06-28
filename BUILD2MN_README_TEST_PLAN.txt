AtariHelp.eu EMU-10 BUILD2MN_SEGA_REAL_CORE_BRIDGE_STAGE1

ZAKLAD:
- vychazi z BUILD2MM_SEGA_AHSAVE_DOWNLOAD_LOG_SAFE
- hlavni N&P VISION menu zustava stejne
- EMULATOR -> Atari/VBXE EMU2 zustava bez zasahu
- spodni leve ATARI / 8-BIT LEGACY -> SEGA MEGA DRIVE modul zustava
- ROM neni v ZIPu ani v APK

CO JE ZMENENE:
1) app/src/main/assets/emu_sega/index.html
   - pridan real-core video canvas 320x224 do monitoru
   - ROM picker po validaci ROM zkusi predat normalizovana ROM data do core bridge
   - A/B/C/START a D-PAD posilaji vstupy do core bridge
   - LOG hlasi core status a zustava pres AHSAVE do Downloads/AtariHelp

2) app/src/main/assets/emu_sega/nap_sega_core_bridge.js
   - nova izolovana bridge vrstva pro realny Mega Drive core
   - hleda adapter: window.NAP_SEGA_REAL_CORE, window.NAP_REAL_SEGA_CORE, window.SegaMDCore, window.GenesisCore
   - podporuje init/loadRom/reset/start/pause/press/release pres vice nazvu metod
   - NEOBSAHUJE fake emulator ani fake Sonic video

DULEZITE:
- BUILD2MN jeste nepridava samotny Mega Drive core.
- Pokud v projektu neni vlozen realny core adapter, po nacteni ROM musi byt videt:
  CORE BRIDGE: ROM_VALIDATED_BUT_CORE_MISSING
- To je spravne. Je to prvni bezpecny krok: kabelaz, canvas, ROM handoff, input handoff, log.

TEST PLAN:
1) Zaloha projektu v GitHub Desktop.
2) Rozbal overlay do korene projektu presne pres app/.
3) Buildni APK / spust v Nox nebo mobilu.
4) Na hlavni obrazovce klikni spodni leve ATARI / 8-BIT LEGACY.
5) V SEGA obrazovce klikni HRY nebo CARTRIDGE a vyber .gen/.bin/.md/.smd ROM.
6) Ocekavany stav bez realneho core:
   - ROM header audit porad funguje.
   - V monitoru je BUILD2MN REAL-CORE BRIDGE STAGE1.
   - Je tam CORE BRIDGE: ROM_VALIDATED_BUT_CORE_MISSING.
   - Zadna hra se NESMI fake spustit.
7) Klikni A/B/C/START/D-PAD.
   - Bez core ma toast psat, ze vstup ceka na realny core adapter.
   - V logu hledej SEGA INPUT BUILD2MN ... CORE_NOT_READY.
8) Klikni ULOZENE.
   - Log se ma ulozit do Downloads/AtariHelp/AtariHelp_SEGA_LOG_YYYYMMDD_HHMMSS.txt.

V LOGU HLEDEJ:
- BUILD2MN_SEGA_REAL_CORE_BRIDGE_STAGE1
- SEGA VIDEO CANVAS BUILD2MN
- SEGA CORE BRIDGE BUILD2MN STATUS
- NO REAL CORE ADAPTER FOUND, pokud core jeste neni vlozen
- SEGA ROM HEADER BUILD2MN
- ROM_VALIDATED_BUT_CORE_MISSING
- SEGA INPUT BUILD2MN DOWN/UP ... CORE_NOT_READY

KODY JSOU ZMENENE:
- app/src/main/assets/emu_sega/index.html
- app/src/main/assets/emu_sega/nap_sega_core_bridge.js

KODY JSOU STEJNE:
- hlavni app/src/main/assets/index.html je pribalen jen pro jistotu a routing zustava z BUILD2MM
- PNG skin sega_megadrive_screen.png je pribalen beze zmen
- Atari/VBXE/EMU2 neni soucasti tohoto overlaye a neni menen

DALSI KROK PO TESTU:
- Pokud BUILD2MN potvrdi ROM picker + log + input + no-fake status, dalsi build bude BUILD2MO.
- BUILD2MO ma vlozit/napojit konkretni realny Mega Drive core adapter do teto bridge vrstvy.
