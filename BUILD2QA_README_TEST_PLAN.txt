BUILD2QA TEST PLAN

1) Nasad overlay pres GitHub Desktop.
2) Nech GitHub Actions vytvorit APK.
3) Pokud build spadne, posli cervene radky kolem BUILD2QA / CMake / ninja.
4) Pokud build projde:
   - SEGA -> C++ CORE
   - vyber Sonic/Aladdin pres normalni cartridge
   - sleduj monitor
   - zkus DPAD/A/B/C/START
   - C++ AUDIO
   - ULOZENE -> poslat log + screenshot

Ocekavane log markery:
- BUILD2QA_SEGA_NATIVE_CPP_OFFLINE_VENDOR_FIRST_CORE_STAGE91
- REAL_CORE_PRESENT=YES
- vendor_offline=local ZIP sources
- REAL_CORE_LOAD_OK
- core=ClownMDEmu-core offline/vendor linked by CMake

KDYZ obraz zustane pattern:
- neni to konec; log rekne jestli real core load OK a jestli render callback dal scanline.
