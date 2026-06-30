BUILD2PQ_SEGA_NATIVE_CPP_INPUT_AUDIO_PROOF_STAGE81

DUZELITE:
- Tohle jeste NENI hotovy Sega emulator.
- Je to druhy native C++ proof po BUILD2PP.
- BUILD2PP overil: Sega stranka -> Java Activity -> JNI -> C++ -> ROM header/checksum -> native render pattern.
- BUILD2PQ pridava: C++ input stav + C++ generovana PCM audio data.
- WebView Sega zustava beze zmen jako zaloha.
- Zadny fake gameplay, zadna ROM v APK.

KODY JSOU ZMENENE:
ANO.

ZMENENE SOUBORY:
- app/build.gradle
- app/src/main/AndroidManifest.xml
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativeSegaProofActivity.java
- app/src/main/cpp/CMakeLists.txt
- app/src/main/cpp/nap_sega_native_proof.cpp
- app/src/main/assets/emu_sega/index.html

CO TESTOVAT:
1. Nasad ZIP overlay pres GitHub Desktop.
2. Build APK.
3. Otevri Sega obrazovku.
4. Klikni C++ TEST.
5. Musi se otevrit SEGA C++ TEST / BUILD2PQ.
6. Klikni VYBRAT ROM a vyber Sonic/Aladdin .gen/.bin/.md.
7. Musi se zobrazit ROM info, checksum a BUILD2PQ native proof text.
8. Drz C++ tlacitka UP/DOWN/LEFT/RIGHT/A/B/C/START.
   - Barevny native test pattern musi reagovat pohybem/baru/rychlosti.
   - LIVE radek musi ukazovat C++ INPUT stav.
9. Klikni C++ AUDIO TEST.
   - Ma se ozvat kratky cisty 440Hz ton.
   - Tohle zatim neni Sega audio, jen test cesty C++ PCM data -> Android AudioTrack.

CO POSLAT ZPET:
- Screenshot BUILD2PQ po vyberu ROM.
- Napsat: C++ INPUT reaguje ano/ne.
- Napsat: C++ AUDIO TEST ton je cisty / chrci / neslysim.
- Napsat: nativePatternFPS priblizne kolik v LIVE radku.

KDYZ SPADNE BUILD:
- Poslat screenshot chyby z Android Studio / Gradle.
- Pravdepodobny problem je chybejici NDK/CMake.

DALSIR SMER:
Pokud BUILD2PQ projde, dalsi vetev uz muze zacit realne vkladat Sega C++ core stub/runner misto pouheho test patternu.
