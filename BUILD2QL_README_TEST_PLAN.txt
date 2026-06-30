BUILD2QL_SEGA_NATIVE_CPP_AUDIO_SYNC_PSG_LANDSCAPE_FIX_STAGE102

KODY JSOU ZMENENE: ano

CIL:
- Navazuje na prvni uspesny native C++ core render (2QJ/2QK smer).
- Oprava zvuku: nizka latence, PSG mix, restart audio streamu pri nove ROM.
- Oprava landscape na mobilu: nativni C++ obraz uz nema prekryt joystick/tlacitka pri otoceni.

ZMENA:
1) C++ audio:
   - 2QK posilal hlavne FM audio a FIFO mohlo narust, proto vznikala prodleva/ozvena.
   - 2QL micha FM + PSG + PCM/CDDA do pull-time mono vystupu.
   - FIFO ma low-latency target 768 frames a hard cap 4096 frames.
   - Kdyz FIFO utece, stare audio se zahazuje, aby nebylo 1-2 s pozadu.

2) Java AudioTrack:
   - chunk snizen z 1024 na 256 frames.
   - low-latency AudioTrack performance mode na Android 8+.
   - pri nove ROM se AudioTrack restartuje, aby nezustal stary buffer z predchozi hry.

3) Landscape native view:
   - native Android view je nad WebView, proto v 2QK prekryl HTML joystick/tlacitka.
   - v landscape 2QL zmensuje native rect do horni gameplay casti a nechava touch ovladani viditelne.

TEST:
1. Nainstaluj overlay.
2. SEGA -> C++ CORE.
3. Vyber Sonic USA/Europe.
   - sleduj: zvuk skoku, cinknuti prstynku, zpozdeni, chrchani.
4. Vyber Aladdin Europe.
   - sleduj: svist mece, zpozdeni, chrchani.
5. Otestuj S8 portrait a landscape.
   - pri otoceni musi byt videt ovladani/joystick nebo musi aspon reagovat.
6. Uloz LOG pres ULOZENE.

LOG MARKERY:
BUILD2QL_SEGA_NATIVE_CPP_AUDIO_SYNC_PSG_LANDSCAPE_FIX_STAGE102
NATIVE_AUDIO_STREAM_START_LOW_LATENCY
NATIVE_AUDIO_PULL_LOW_LAT
realCoreStatus audio_mix=FM+PSG+PCM lowLatency=YES
NATIVE_RECT_DPR ... landscapeSafeControls=YES

NECEKAT:
- Dokonaly zvuk. Tohle je prvni realna audio synchronizacni oprava po QK.
- PSG/FM mix muze chtit dalsi hlasitostni pomery.
