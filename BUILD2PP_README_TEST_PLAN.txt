AtariHelp.eu EMU-10 / SEGA
BUILD2PP_SEGA_NATIVE_CPP_PROOF_BUTTON_STAGE80

DULEZITE:
- Tohle NENI hotovy Sega emulator.
- Tohle NENI fake gameplay.
- Soucasna WebView Sega zustava v appce jako zaloha.
- Cilem je prvni realny hybridni krok: Sega stranka -> Java bridge -> Android native Activity -> JNI -> C++ knihovna.

CO JE ZMENENE:
1) app/build.gradle
   - pridany CMake / externalNativeBuild pro C++ JNI knihovnu.
2) app/src/main/AndroidManifest.xml
   - pridana NativeSegaProofActivity.
3) app/src/main/java/eu/atarihelp/emu10/MainActivity.java
   - pridany AHNATIVE JavascriptInterface.
4) app/src/main/java/eu/atarihelp/emu10/NativeSegaProofActivity.java
   - nova testovaci native obrazovka.
5) app/src/main/cpp/CMakeLists.txt
   - CMake build soubor.
6) app/src/main/cpp/nap_sega_native_proof.cpp
   - C++ JNI proof: vraci build string, analyzuje Mega Drive ROM header/checksum, renderuje native test pattern.
7) app/src/main/assets/emu_sega/index.html
   - pridano tlacitko C++ TEST na Sega stranku.

TEST PLAN:
1. Nasad ZIP overlay pres GitHub Desktop.
2. Build APK.
3. Otevri aplikaci.
4. Otevri SEGA cast.
5. Klikni na nove tlacitko C++ TEST.
6. Musi se otevrit nova landscape obrazovka SEGA C++ TEST / BUILD2PP.
7. Dej VYBRAT ROM PRO C++ TEST a vyber .gen/.bin/.md.
8. Ocekavany vysledek:
   - obrazovka ukaze BUILD2PP NATIVE C++ PROOF OK,
   - ukaze bytes, FNV1A32, Mega Drive header, title, region, checksum,
   - vlevo bezi hybny barevny native test pattern generovany z C++.

CO TESTOVAT NA NOX / S8 / A12:
- jestli se C++ TEST otevrel,
- jestli ROM jde vybrat,
- jestli C++ vrati header info,
- jestli se test pattern hybe plynule,
- jestli zpet do appky funguje.

POKUD BUILD SPADNE:
- Posli presny error/screenshot.
- Nejpravdepodobnejsi chyba: chybi Android NDK nebo CMake v Android Studio SDK Manageru.
- To neni chyba kodu emulatoru, ale chybejici C++ build nastroj.

CO TO ZNAMENA DAL:
- Kdyz BUILD2PP projde, mame dokaz, ze hybrid C++ cesta v projektu funguje.
- Dalsi krok bude teprve vybrat realny Sega C/C++ core s vhodnou licenci a napojit obraz/audio/input.
