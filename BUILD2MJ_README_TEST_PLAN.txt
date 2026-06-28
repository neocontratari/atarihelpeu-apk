BUILD2MJ_SEGA_ROM_PICKER_CORE_PREP_SAFE

STATUS:
- Navazuje na BUILD2MI router + SEGA obrazovku 1:1.
- Sonic ROM neni v ZIPu ani APK.
- Bez fake spusteni hry.
- Pridan realny lokalni ROM picker a audit Mega Drive hlavicky/checksumu.

ZMENA:
- app/src/main/assets/emu_sega/index.html
  - HRY / SBIRKA / CARTRIDGE SLOT oteviraji file picker.
  - Podporuje .gen/.bin/.md/.smd.
  - Cte SEGA header, title, region, serial, checksum, CRC32.
  - Monitor prekryje stavem ROM CHECK po vyberu souboru.
- app/src/main/assets/index.html zustava z BUILD2MI: EMULATOR -> emu_vbxe, footer ATARI -> emu_sega.

LOG MARKERY:
- AtariHelp.eu EMU-10 BUILD2MJ_SEGA_ROM_PICKER_CORE_PREP_SAFE pripraven
- SEGA ROM PICKER BUILD2MJ
- SEGA ROM HEADER BUILD2MJ
- SEGA CORE NOT FAKE BUILD2MJ

TEST:
1) Hlavni N&P VISION zustane stejna.
2) EMULATOR -> Atari/VBXE.
3) Spodni leve ATARI -> SEGA obrazovka.
4) V SEGA klikni HRY nebo SBIRKA nebo cartridge slot.
5) Vyber svuj lokalni Sonic .gen.
6) Ocekavani: v monitoru se ukaze ROM CHECK OK, title SONIC THE HEDGEHOG, region JUE, checksum.

DULEZITE:
- Toto jeste neni Mega Drive core. Je to poctiva priprava pro realny core bez fake obrazu.
- Dalsi krok: vlozit/napojit open-source Mega Drive core nebo nativni WASM/JNI core.
