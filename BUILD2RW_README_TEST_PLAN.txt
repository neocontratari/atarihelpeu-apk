BUILD2RW_SEGA_NATIVE_CPP_ONLY_SAFE_PLAYABLE_PASSIVE_AUDIT_STAGE139

Ucel:
- Presne podle predavaciho planu: zaklad je hratelny BUILD2RV, pridany je POUZE pasivni audit.
- Tenhle build NIC neopravuje naslepo. Meri a loguje, aby dalsi build (RX) opravoval
  konkretni namerenou pricinu (S8 chrceni/zpomalovani, EU vs USA, levy pruh), ne odhad.

CO SE NEZMENILO (garantovane stejne jako RV):
- Region detekce: JUE zustava US_NTSC_OVERSEAS, E-only zustava PAL jako v RV.
- Audio FIFO struktura, target/max/low-water, governor, no-starve, catchup: beze zmeny.
- Frame clock (NTSC 16.67ms / PAL 20ms): beze zmeny.
- Gain/basy/limiter: beze zmeny.
- Render cesta, left-boundary shift, TextureView: beze zmeny.
- Java wrapper zustava vypnuty, C++ only.

CO JE PRIDANE (jen mereni a logovani):
1) C++ 10s FIFO okno: fifo10sMin/Max/Avg, underruns10s, pull10sUsAvg/Max, pulls10s.
2) C++ left-edge audit: leftBoundary/rightBoundary/coreScreen z prvniho scanline,
   pocet zmen boundary, checksum levych 16 px prostredniho radku framebufferu,
   leftColBlackFrames vs leftColNonBlackFrames.
   -> Kdyz je pruh videt na obrazovce, ale leftColBlackFrames roste a FNV je stabilni,
      chyba je v Java/TextureView scalingu, ne v C++ framebufferu. A naopak.
3) C++ regionClockRW: realne nastaveny tv standard, framePeriodNs, fmSrcRate, psgSrcRate.
   -> Dukaz, cim core OPRAVDU bezi, ne jen text labelu.
4) C++ nativeMalloc audit (mallinfo): rust nativniho heapu v case.
5) Java PASSIVE_AUDIT_RW kazdych 10 s: javaHeapUsed/Total/Max, nativeHeapAlloc/Total,
   GC count + GC time (ART), AudioTrack.getUnderrunCount() (realny Android citac underrunu),
   activeAudioTracks, audioGen, romGen, drawCounter, lastRenderCostMs, perfMode
   + zkraceny C++ audit blok.
6) Periodicky NATIVE_AUDIO_PULL log nove obsahuje audioTrackUnderruns.
7) Log ring zvetsen z 20 KB na 100 KB, aby v ulozenem logu prezil 10-15 minutovy S8 test.

ZMENENE SOUBORY:
- app/src/main/cpp/nap_sega_native_proof.cpp
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/assets/emu_sega/index.html
NativeSegaCoreBridge.java je BEZE ZMENY (zadna nova JNI metoda; audit jde pres realCoreStatus).

TEST PLAN (po lopate):
1) Nox - Sonic JUE:
   - CARTRIDGE -> vybrat ROM.
   - Ocekavani: hra bezi PRESNE jako v RV (stejny obraz, stejny zvuk). Zadny modry monitor.
   - V logu musi byt: BUILD2RW_SEGA_NATIVE_CPP_ONLY_SAFE_PLAYABLE_PASSIVE_AUDIT_STAGE139,
     PASSIVE_AUDIT_RW_START, a po ~10 s prvni radek PASSIVE_AUDIT_RW tick=1.
   - Nech bezet aspon 2-3 minuty, pak SAVE LOG a posli.

2) S8 - DLOUHY TEST (tohle je hlavni ukol tohoto buildu):
   - USA ROM (ta, co hrala cistě): nech hrat 10-15 minut, nesahat na telefon.
   - Pak SAVE LOG a posli.
   - EU ROM (ta, co chrcela): to same, 10-15 minut, SAVE LOG, posli.
   - Kdyz se hudba zacne zpomalovat, poznamenat priblizne V KOLIKATE MINUTE.

3) Nox - levy pruh:
   - Spustit hru, kde je pruh videt, nechat chvili bezet, SAVE LOG + screenshot.

LOG MARKERY:
- BUILD2RW_SEGA_NATIVE_CPP_ONLY_SAFE_PLAYABLE_PASSIVE_AUDIT_STAGE139
- audio_mode=FM_PSG_ZEROED_RV_UNCHANGED_PASSIVE_AUDIT_RW
- PASSIVE_AUDIT_RW_START / PASSIVE_AUDIT_RW tick=N
- regionClockRW label=... tv=... framePeriodNs=...
- leftAuditRW leftBoundary=... leftColBlackFrames=...
- auditRW=PASSIVE_ONLY fifo10sMin=... underruns10s=...
- NATIVE_AUDIO_PULL_RW_PASSIVE_AUDIT ... audioTrackUnderruns=...
- frameReady=YES, videoCaptured>0, nativeActiveAudioTracks=1 (stejne jako RV)

CO SE NESMI OBJEVIT:
- BUILD2RT / BUILD2RU markery, FM_PSG_REGION_MEMORY_AUDIT_RT, FM_PSG_REGION_FIX_VISIBLE_FRAME_RU,
  audioRingAudit=FIXED_RING_NO_DEQUE, JUE_APP_DEFAULT_EU, modry monitor po ROM loadu.

OCEKAVANY VYSLEDEK:
- Hratelnost identicka s RV. Pokud by RW hrala HUR nez RV, je to chyba a hned to hlas.
- Z ulozenych logu z bodu 2 pujde poprve tvrde rict, CO na S8 roste:
  javaHeap? nativeHeap? fifo backlog? underruny? pull cost? render cost? GC time?
  Podle toho se udela cileny BUILD2RX fix a oddeleny PAL/NTSC test selector.

CO OD TEBE BUDU POTREBOVAT:
- 3-4 ulozene logy podle test planu (Nox JUE, S8 USA dlouhy, S8 EU dlouhy, pripadne Nox pruh + screenshot).
- Nazvy ROM (staci nazvy souboru), abych sparoval header region s chovanim.
