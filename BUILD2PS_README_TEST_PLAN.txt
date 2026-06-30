AtariHelp.eu EMU-10 BUILD2PS_SEGA_NATIVE_CPP_60FPS_TIMING_LOG_STAGE83

UPOZORNENI:
- Toto jeste NENI hotovy Sega emulator.
- Neni zde fake gameplay.
- WebView Sega zustava jako zaloha beze zmen.
- Cil je overit native Java -> JNI -> C++ timing pred vlozenim skutecneho Sega C++ core.

CO JE NOVE PROTI BUILD2PR:
- C++ TEST obrazovka zustava.
- ROM header/checksum/input/audio/log zustava.
- Native proof pattern uz nema test tick 33 ms (~30 FPS), ale 16 ms cil (~60 FPS).
- C++ log obsahuje nativePatternTarget=60fps frameDelayMs=16.

TEST PLAN:
1. Nasad overlay pres GitHub Desktop.
2. Build APK.
3. Sega -> C++ TEST.
4. Vyber Sonic/Aladdin ROM.
5. Sleduj LIVE nativePatternFPS.
6. Zmackni C++ AUDIO TEST.
7. Zmackni par C++ tlacitek.
8. Dej ULOZIT C++ LOG.
9. Posli TXT log a napis:
   - Nox nativePatternFPS = ?
   - S8 nativePatternFPS = ?
   - A12 nativePatternFPS = ?
   - C++ audio cisty/chrci/neslysim
   - C++ input reaguje ano/ne

OCEKAVANI:
- Nox a S8 by se mely drzet vyrazne vys nez 29 FPS, idealne 55-60.
- Pokud A12 nezvladne ani tento native proof, problem je zarizeni/Android/Nox/driver.
- Pokud A12 zvladne native proof, ale WebView Sega ne, dava smysl jit na skutecny C++ core.
