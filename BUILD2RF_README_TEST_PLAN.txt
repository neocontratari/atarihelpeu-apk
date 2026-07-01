AtariHelp.eu EMU-10 - BUILD2RF
SEGA_NATIVE_CPP_ONLY_AUDIO_ROLLBACK_PERF_SWITCH_STAGE122

CO BUILD2RF MENI:
1) Zvuk rollback:
- Vraci C++ FM/PSG callback cestu zpet smerem k RD/QT/QP profilu.
- Odstranuje RE thread_local audio buffer z core callbacku, protoze po testu zhorsil basy / chrasteni.
- AudioTrack profil zustava 48 kHz / chunk 384 / QT-QP charakter.

2) SBIRKA = LOW/HIGH vykonovy prepinac:
- SBIRKA uz neni ROM picker.
- SBIRKA otevira volbu HIGH QUALITY / LOW PERFORMANCE.
- Volba se uklada do localStorage napSegaPerfMode.
- Po volbe se vrati zpet na Sega obrazovku.
- ROM se dal vybira pres CARTRIDGE / CART.

3) HIGH / LOW:
- HIGH QUALITY: plna render prezentace pro Nox a novejsi mobily, bez zbytecne graficke degradace.
- LOW PERFORMANCE: omezi jen prezentacni render tempo na starsich mobilech S8/A12, core/audio timing zustava realny.
- Loguje NATIVE_PERF_MODE_RF a NATIVE_RENDER_PERF_RF vcetne perfMode.

4) Landscape ovladac:
- A/B/C jsou v landscape nad sebou vpravo dole.
- Vetsi touch zony zustavaji.
- DPAD zustava vlevo.

5) Zachovano:
- Sega je C++ only, Java/WebView wrapper zustava vypnuty.
- C++ CORE tlacitko/cesta zustava odstranena.
- Region ROM se neprepisuje.
- Nox navrat z Atari 130XE zustava zachovan.

TEST PLAN:
1. Workflow: overit, ze build projde a vznikne jedna APK.
2. Nox: Sonic -> Atari 130XE -> zpet Sega -> Sonic/Aladdin.
   Ocekavani: zadna cerna obrazovka se zvukem, Sega UI ciste bez C++ CORE fleku.
3. S8 HIGH: SBIRKA -> HIGH QUALITY -> CARTRIDGE -> Sonic 5 minut.
   Ocekavani: lepsi grafika/plynulost nez LOW, zvuk bez RE basoveho chrasteni.
4. S8 LOW: SBIRKA -> LOW PERFORMANCE -> Sonic 10-15 minut.
   Ocekavani: mensi zpomalovani/prehriivani; pokud se zpomali, hned ulozit LOG.
5. S8 multi-ROM: Sonic -> Aladdin -> Samurai -> Sonic.
   Ocekavani: po vice ROM se nema periodicky zhorsovat video/zvuk. Pokud ano, log musi ukazat perfMode, render FPS a audio underruns.

LOG MARKERY:
BUILD2RF_SEGA_NATIVE_CPP_ONLY_AUDIO_ROLLBACK_PERF_SWITCH_STAGE122
BUILD2RF PERF_MODE_SET mode=HIGH/LOW
NATIVE_PERF_MODE_RF mode=HIGH/LOW
NATIVE_AUDIO_STREAM_START_RF_AUDIO_ROLLBACK_QT
NATIVE_RENDER_PERF_RF ... perfMode=HIGH/LOW
NATIVE_TEXTURE_FRAME_RF ... gameDst=...
