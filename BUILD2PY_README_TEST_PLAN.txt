AtariHelp.eu EMU-10 BUILD2PY_SEGA_NATIVE_CPP_CLOWNMDEMU_CORE_FIRST_BOOT_STAGE89

KODY JSOU ZMENENE: ANO

SMER:
- Normalni Sega obrazovka zustava.
- Zadna dalsi C++ aktivita/okno.
- C++ obraz zustava v monitoru jako u BUILD2PV.
- Joystick/tlacitka/cartridge picker zustavaji v normalnim UI.
- Do C++ slotu je zapojen realny ClownMDEmu-core pres CMake FetchContent.
- Tohle uz neni dalsi pattern test. Pattern zustava jen fallback, kdyz core neni nahrany nebo build core neprojde.
- ROM neni v APK.

DULEZITE:
- Pokud GitHub Actions spadne v CMake/NDK/FETCH, neposilej screenshot summary, ale cervene radky kolem chyby.
- Pokud build projde, testuj normalni Sega obrazovku: C++ CORE -> vybrat Sonic/Aladdin -> sleduj monitor -> ovladani -> ULOZENE.

OCEKAVANE LOG MARKERY:
BUILD2PY_SEGA_NATIVE_CPP_CLOWNMDEMU_CORE_FIRST_BOOT_STAGE89
REAL_CORE_PRESENT=YES
core=ClownMDEmu-core
REAL_CORE_LOAD_OK
video=first native Iterate/render attempt enabled

KDYZ UVIDIS JEN BAREVNY PATTERN:
- To znamena, ze C++ bridge bezi, ale real core se nenahrava nebo nedal render frame.
- Posli log ULOZENE.

KDYZ SE OBJEVI HRA NEBO JINY REALNY OBRAZ:
- Posli screenshot + log.
- Pak se jde na audio mixing a frame pacing.
