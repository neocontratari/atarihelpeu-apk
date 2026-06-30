AtariHelp.eu EMU-10 BUILD2QF_SEGA_NATIVE_CPP_EXPLICIT_CORE_STEP_STAGE96

CIL:
- Po BUILD2QE uz mame stabilni cerny monitor + ROM log bez padu.
- BUILD2QF nedela auto core load po vyberu ROM.
- ROM se pouze staged do C++ pameti.
- Tlacitko C++ CORE se po zapnuti zmeni na C++ STEP.
- Kazdy stisk C++ STEP pusti presne jeden nativni krok:
  1) constant_init
  2) initialise
  3) set_cartridge
  4) hard_reset
  5) iterate_one

KODY JSOU ZMENENE: ANO.

NEMENENO:
- normalni Sega UI
- joystick / A/B/C/START
- cartridge picker
- WebView Sega fallback
- pozice C++ monitoru z 2PV/2QE

TEST:
1. Nasad overlay.
2. Build APK.
3. Otevri SEGA.
4. Dej C++ CORE.
5. Vyber Aladdin/Sonic pres normalni cartridge.
6. Dej ULOZENE hned po vyberu ROM, pokud chces kontrolni log staged.
7. Mackej C++ STEP po jednom stisku.
8. Po kazdem stisku sleduj, zda appka zije.
9. Dej ULOZENE a posli log.

LOG MARKERY:
- BUILD2QF_SEGA_NATIVE_CPP_EXPLICIT_CORE_STEP_STAGE96
- REAL_CORE_ROM_STAGED_FOR_EXPLICIT_STEP
- REAL_CORE_EXPLICIT_STEP_BEGIN stage=0
- REAL_CORE_STEP_OK constant_init
- REAL_CORE_STEP_OK initialise
- REAL_CORE_STEP_OK set_cartridge
- REAL_CORE_STEP_OK hard_reset
- REAL_CORE_STEP_OK iterate_one

KDYZ SPADNE:
- napis presne po kterem C++ STEP stisku to spadlo.
