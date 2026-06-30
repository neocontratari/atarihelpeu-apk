BUILD2QD_SEGA_NATIVE_CPP_SIGNAL_GUARD_NO_CRASH_STAGE94

Test:
1. Nasad overlay.
2. Nech vyrobit APK.
3. Otevri SEGA.
4. Stiskni C++ CORE.
   Ocekavani: zadne behajici kostky, zadny stredovy ctverecek, jen tmavy monitor s jemnym modrym okrajem.
5. Stiskni HRY / SBIRKA / CART a vyber Sonic nebo Aladdin.
   Ocekavani: appka NESMI spadnout.
6. Dej ULOZENE a posli log.

Hledane log markery:
- BUILD2QD_SEGA_NATIVE_CPP_SIGNAL_GUARD_NO_CRASH_STAGE94
- pattern=OFF
- REAL_CORE_ROM_STAGED
- REAL_CORE_STEP constant_init
- REAL_CORE_STEP initialise
- REAL_CORE_STEP set_cartridge
- REAL_CORE_STEP hard_reset
- bud REAL_CORE_LOAD_OK_GUARDED_NO_AUTO_ITERATE
- nebo REAL_CORE_NATIVE_SIGNAL_CAUGHT=SIGSEGV/SIGBUS/SIGABRT

Kdyz appka i tak spadne:
- poslat screenshot okamziku padu
- poslat GitHub APK build run link jen pokud neprosel build
- napsat presne: spadlo po C++ CORE / po vyberu ROM / po tlacitku
