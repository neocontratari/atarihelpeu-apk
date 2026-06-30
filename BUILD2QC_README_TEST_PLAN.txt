BUILD2QC TEST PLAN

1) Nasad overlay pres GitHub Desktop.
2) Nech vyrobit APK.
3) Otevri SEGA.
4) Dej C++ CORE.
5) V monitoru nesmi litat behajici kostky. Ma byt staticky tmavy native guard obraz.
6) Vyber Aladdin/Sonic pres normalni cartridge.
7) Aplikace NESMI spadnout.
8) Dej ULOZENE a posli log.

Hledane markery v logu:
- BUILD2QC_SEGA_NATIVE_CPP_CORE_CRASH_GUARD_NO_PATTERN_STAGE93
- REAL_CORE_LOAD_OK_GUARDED
- REAL_CORE_RENDER_GUARDED_NO_AUTO_ITERATE
- pattern=OFF
- no running cubes
- checksum match: YES

Pokud apk spadne i v 2QC:
- poslat GitHub/Android crash log, protoze by pad nebyl v Iterate/render guardu, ale uz v ClownMDEmu_Initialise/HardReset.
