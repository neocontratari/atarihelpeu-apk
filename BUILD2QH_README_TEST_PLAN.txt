AtariHelp.eu EMU-10 BUILD2QH_SEGA_NATIVE_CPP_REAL_CORE_RENDER_BUILD_FIX_STAGE98

KODY JSOU ZMENENE: ANO

OPRAVA PROTI BUILD2QG:
- BUILD2QG spadl pri skladani/workflow.
- Nalezena konkretni C++ compile chyba v nap_sega_native_proof.cpp:
  nap_real_load_rom_bytes() volal nap_real_setup_cfg_callbacks() pred deklaraci.
- BUILD2QH pridava forward declaration:
  static void nap_real_setup_cfg_callbacks();
- C++ soubor byl overen lokalnim host C++ compile testem se stub JNI hlavickami.

ZUSTAVA:
- normalni Sega obrazovka
- zadne dalsi C++ okno
- C++ in-place monitor
- vendor ClownMDEmu-core offline source
- joystick/tlacitka/cartridge UI beze zmen

TEST:
1. Nasad BUILD2QH overlay.
2. GitHub Actions musi projit buildem.
3. Pokud build spadne, poslat prvni cerveny compiler error, ne jen summary.
4. Pokud build projde: SEGA -> C++ CORE -> vyber Aladdin/Sonic -> screenshot + ULOZENE log.

HLEDAT V LOGU:
BUILD2QH_SEGA_NATIVE_CPP_REAL_CORE_RENDER_BUILD_FIX_STAGE98
REAL_CORE_LOAD_OK
REAL_CORE_RENDER_OK
