BUILD2PT_SEGA_NATIVE_CPP_IN_PLACE_NORMAL_UI_STAGE84

CIL:
- Ukoncit samostatne C++ test okno.
- Dat C++ proof primo do normalni Sega emulator obrazovky.
- Pouzit existujici UI: CARTRIDGE/SBIRKA, DPAD, A/B/C/START, disketa ULOZENE.
- Nehonit dalsi 60fps testy. Jde o cestu k hratelnosti.

TEST:
1. Nasad ZIP overlay pres GitHub Desktop.
2. Build APK.
3. Otevri normalni SEGA emulator.
4. Stiskni tlacitko C++ CORE.
   - NESMI se otevrit dalsi native test okno.
   - V monitoru normalni Sega obrazovky se ma objevit C++ pattern.
5. Pres CARTRIDGE/SBIRKA vyber Sonic nebo Aladdin .gen/.bin/.md.
   - ROM se ma poslat do C++.
   - V logu se ma objevit NATIVE_ROM_TO_CPP a checksum info.
6. Zkus normalni DPAD a A/B/C/START.
   - C++ pattern musi reagovat.
   - Zadne extra C++ tlacitko okno uz tam nema byt.
7. Stiskni C++ CORE podruhe.
   - Pusti se C++ AUDIO TEST.
8. Stiskni disketu ULOZENE.
   - Ma ulozit normalni Sega log.
   - Pokud je C++ mode aktivni, ulozi se i AtariHelp_SEGA_CPP_INPLACE_LOG_BUILD2PT_*.txt.

CO POSLAT ZPET:
- Screenshot normalni Sega obrazovky s C++ patternem.
- C++ in-place TXT log z Downloads/AtariHelp.
- Kratce:
  C++ je v normalni Sega obrazovce ano/ne
  ROM parser funguje ano/ne
  DPAD/A/B/C/START reaguji ano/ne
  C++ audio test slysim cisty/chrci/neslysim

DULEZITE:
- Neni to hotovy emulator.
- Je to posledni proof krok pred vymenou patternu za realny Sega C++ core.
- WebView Sega zustava zaloha.
