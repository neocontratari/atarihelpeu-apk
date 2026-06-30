BUILD2PR_SEGA_NATIVE_CPP_LOG_EXPORT_STAGE82

CIL:
- Navazuje na BUILD2PQ.
- WebView Sega zustava jako zaloha beze zmen.
- Native C++ test porad neni fake gameplay ani hotovy emulator.
- Novinka: C++ TEST obrazovka ma vlastni tlacitko ULOZIT C++ LOG.
- Log uklada ROM header/checksum, device info, input udalosti, audio test stav a nativePatternFPS.

CO JE ZMENENE:
- NativeSegaProofActivity.java:
  - title BUILD2PR
  - pridano ULOZIT C++ LOG
  - pridany native C++ log do Downloads/AtariHelp
  - loguje ROM picker, ROM read, native parser, input, audio tone, device info
- nap_sega_native_proof.cpp:
  - build string BUILD2PR + native log export OK
- index.html / MainActivity / Gradle:
  - build label zvednut na BUILD2PR

KODY JSOU ZMENENE: ANO

TEST:
1. Nasad overlay pres GitHub Desktop.
2. Build APK.
3. Otevri Sega cast.
4. Klikni C++ TEST.
5. Vyber ROM Sonic/Aladdin.
6. Otestuj DPAD/A/B/C/START v C++ testu.
7. Klikni C++ AUDIO TEST.
8. Klikni ULOZIT C++ LOG.
9. Posli vytvoreny TXT z Downloads/AtariHelp:
   AtariHelp_SEGA_CPP_LOG_BUILD2PR_YYYYMMDD_HHMMSS.txt

OCEKAVANE:
- SEGA C++ TEST / BUILD2PR
- BUILD2PR NATIVE C++ INPUT/AUDIO/LOG PROOF OK
- ROM header/checksum porad funguje
- C++ input meni pattern
- audio test pipne
- C++ log se ulozi do Downloads/AtariHelp

POZOR:
- Tohle porad neni Sega emulator.
- Tohle je technicky most Java -> JNI -> C++ -> ROM/input/audio/log.
