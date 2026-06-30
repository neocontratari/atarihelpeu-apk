AtariHelp.eu EMU-10 BUILD2QJ_SEGA_NATIVE_CPP_CORE_BIGSTACK_THREAD_STACK_FIX_STAGE100

KODY JSOU ZMENENE: ano

CIL:
- Opravit pad aplikace po vyberu ROM v C++ core modu.
- Zachovat normalni Sega obrazovku, zadne dalsi C++ okno.
- Zachovat joystick/tlacitka/cartridge picker.
- Realny ClownMDEmu-core zustava offline/vendor v APK buildu.

CO JE OPRAVENO PROTI 2QI:
- ClownMDEmu ma velky stav (>1 MB). Predchozi kod pouzival g_real = NapRealCoreState(), coz muze vytvorit velky docasny objekt a shodit Android/WebView stack.
- Reset core stavu je ted in-place: nap_real_reset_state_locked().
- Real core worker uz nejede pres std::thread s default Android stackem.
- Worker jede pres pthread s explicitnim 8 MB stackem.
- onDraw stale jen kopiruje cached framebuffer.
- WebView/JavaBridge ani UI draw thread primo nevolaji ClownMDEmu_Iterate.

TEST:
1. Nasad overlay.
2. Build APK.
3. Otevri SEGA.
4. Dej C++ CORE.
5. Vyber Aladdin/Sonic pres cartridge.
6. Pokud spadne: napis presne, ze spadlo po vyberu ROM.
7. Pokud nespadne: pockej 5-10 s, zkus tlacitka, dej ULOZENE a posli log + screenshot.

LOG MARKERY:
- BUILD2QJ_SEGA_NATIVE_CPP_CORE_BIGSTACK_THREAD_STACK_FIX_STAGE100
- REAL_CORE_THREAD_START_OK
- threading=DEDICATED_NATIVE_WORKER_THREAD_BIGSTACK_8MB
- reset=no_stack_temporary
- REAL_CORE_WORKER_ALIVE=YES
