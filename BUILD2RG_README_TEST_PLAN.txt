AtariHelp.eu EMU-10 BUILD2RG
SEGA_NATIVE_CPP_ONLY_AUDIO_FIRST_TIMING_LOGDEDUP_STAGE123

CIL BUILDU:
- Nox ponechat funkcni.
- S8/A12 resit skutecny problem z RF: po case zacal hladovet audio FIFO, zvuk zpomalil a nasledne spadl stream videa.
- Neresit dalsi skin/zaplaty. C++ CORE zustava odstranene, Sega UI zustava ciste.

ZMENY:
1) Audio-first timing:
   - Java AudioTrack uz vzdy dostava plny audio chunk 384 vzorku.
   - Native pullAudio uz pri underrunu doplni buffer de-click dozvukem, ale Java uz nepise jen cast `got`.
   - RF psal pri partial got napr. 150 pouze 150 frames, cimz AudioTrack hladovel.

2) Vetsi bezpecny audio buffer:
   - HIGH: vetsi prefill a vyssi setBufferSizeInFrames nez RF.
   - LOW: jeste vetsi buffer / prefill pro S8/A12.
   - Zvukovy charakter FM/PSG zustava QT/QP rollback, nemeni se pomery basu/PSG.

3) C++ core audio backlog catch-up:
   - Native core sleduje audioBacklog.
   - Kdyz backlog spadne pod waterline, core smi udelat kratky limitovany catch-up burst.
   - Cilem je zabranit tomu, aby audio hlad vyvolal trvale zpomaleni zvuku a pak videa.

4) LOW/HIGH nyni jde i do C++:
   - AHNATIVE.setPerformanceMode vola NativeSegaCoreBridge.setPerformanceMode().
   - C++ podle mode meni audio target/max/lowWater.

5) Log de-dup:
   - Jedno zmacknuti LOG nesmi vytvorit 4 soubory.
   - JS ma 2.4s zamek, Java AHNATIVE.saveLog ma 2.2s zamek.

TEST:
1. Workflow: musi projit a vytvorit jednu APK.
2. Nox: Sonic -> Atari 130XE -> zpet Sega -> Sonic/Aladdin.
3. S8 HIGH: SBIRKA -> HIGH -> CART -> Sonic 5 minut.
4. S8 LOW: SBIRKA -> LOW -> Sonic 15 minut.
5. Pri prvnim zpomaleni hned ulozit log.

V LOGU HLEDEJ:
BUILD2RG_SEGA_NATIVE_CPP_ONLY_AUDIO_FIRST_TIMING_LOGDEDUP_STAGE123
NATIVE_AUDIO_STREAM_START_RG_AUDIO_FIRST_QT
NATIVE_AUDIO_PULL_RG_AUDIO_FIRST_QT
NATIVE_PERF_MODE_OK_RG mode=LOW/HIGH target= max= lowWater=
REAL_CORE_RENDER_OK_WORKER_THREAD_RG ... audioBacklog= perfLow=
NATIVE_RENDER_PERF_RG
SAVE_LOG_DEDUP_RG / BUILD2RG LOG_SAVE_DEDUP pouze pokud se log tlacitko zmackne vic krat rychle za sebou

POZNAMKA:
Pokud se S8 porad po 10-15 minutach zpomali, v dalsim kroku uz bude potreba snizit emulacni takt / frame pacing adaptivne pro LOW, protoze RF log ukazal, ze render sam o sobe byl levny, ale audio FIFO hladovelo.
