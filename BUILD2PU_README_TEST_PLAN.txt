BUILD2PU_SEGA_NATIVE_CPP_IN_PLACE_BUILD_FIX_STAGE85

CIL:
- Opravit build po BUILD2PT.
- Zadne dalsi samostatne C++ okno.
- C++ proof primo v normalni Sega obrazovce.

TEST:
1. Nasad overlay pres GitHub Desktop.
2. Pockej na GitHub Actions build.
3. Pokud build projde, nainstaluj APK.
4. Otevri SEGA.
5. Stiskni C++ CORE.
6. Nesmí se otevrit dalsi C++ aktivita/okno.
7. V normalnim Sega monitoru se ma objevit C++ barevny pattern.
8. Pres CARTRIDGE/SBIRKA vyber Sonic/Aladdin .gen.
9. DPAD/A/B/C/START musi menit C++ pattern.
10. Druhy stisk C++ CORE pusti C++ audio test.
11. Disketa ULOZENE ulozi bezny Sega log + C++ in-place log.

CO POSLAT ZPET:
- screenshot, jestli build projde nebo spadne.
- pokud spadne, poslat plny detail chyby z radku FAILURE / Execution failed.
- pokud projde, poslat C++ in-place TXT log z Downloads/AtariHelp.

POZOR:
- Tohle porad neni hotovy Sega core/gameplay.
- WebView Sega zustava zaloha.
