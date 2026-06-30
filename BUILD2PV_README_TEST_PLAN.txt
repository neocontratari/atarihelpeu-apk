BUILD2PV_SEGA_NATIVE_CPP_IN_PLACE_RECT_DPR_FIX_STAGE86

TEST:
1. Nasad overlay pres GitHub Desktop.
2. Build APK.
3. Otevri SEGA.
4. Stiskni C++ CORE.
5. C++ barevny pattern ma sedet do cerneho monitoru, ne pres logo/nadpis.
6. Vyber ROM pres cartridge/sbirka.
7. Zkus DPAD/A/B/C/START.
8. Dej C++ AUDIO.
9. Dej ULOZENE a posli SEGA LOG.

V LOGU HLEDAT:
- BUILD2PV_SEGA_NATIVE_CPP_IN_PLACE_RECT_DPR_FIX_STAGE86
- NATIVE_RECT_DPR css=... dpr=... native=...
- SET_RECT_OK x=... y=... w=... h=...
- NATIVE_ROM_TO_CPP
- INPUT POST_TO_NATIVE_CPP

CEKANY VYSLEDEK:
- Build projde.
- Neotevira se zadne dalsi C++ okno.
- C++ obraz/pattern je v monitoru a odpovida jeho velikosti.
- Joystick reaguje jako v 2PU.
