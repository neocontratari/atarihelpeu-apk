BUILD2RQ_SEGA_NATIVE_CPP_ONLY_BALANCED_AUDIO_CLOCK_STAGE133

KODY JSOU ZMENENE.

CIL:
- Nevracet se k rozbitemu bass/gong mixu. Zaklad zustava z RO: zeroed FM/PSG scratch buffery pred ClownMDEmu += mixem.
- Neopakovat RP chybu: moc maly FIFO a AudioTrack chunk zpusobily miliony dropu a rozsekany Nox zvuk.
- Udelat stredni stabilni audio-clock: mensi zpozdeni nez RO, ale bez hard orezavani jako RP.

HLAVNI ZMENY:
- Native FIFO target 3072, max 16384, lowWater 1536.
- AudioTrack chunk 384 stereo frames.
- Java AudioTrack uz nenuti PERFORMANCE_MODE_LOW_LATENCY.
- Prefill je stredni, ne obri RO a ne miniaturni RP.
- Pridan audio-clock governor: kdyz FIFO moc ujede dopredu, core jemne pribrzdi misto zahazovani milionu vzorku.
- Loguje clockSleeps, latencyFrames, latencyMs, drops, trimEvents.
- Zeroed FM/PSG buffer zustava. Core LPF zustava ON.
- Single audio path zustava: activeAudioTracks=1.

TEST PLAN:
1) Workflow musi projit a APK vzniknout.
2) Nox / Sonic The Hedgehog (USA, Europe).gen / HIGH:
   - overit, ze neni zpet bass/gong bordel z RN/RM,
   - overit, ze zvuk neni rozsekany jako RP,
   - overit, jestli cinknuti/skok nema ~1s zpozdeni jako RO.
3) S8 / Sonic / HIGH:
   - overit jestli chrasteni/praskani kleslo,
   - overit 2. a 3. intro loop.
4) Pokud zvuk selze, poslat normalni LOG + C++ INPLACE LOG.

LOG MARKERY:
- BUILD2RQ_SEGA_NATIVE_CPP_ONLY_BALANCED_AUDIO_CLOCK_STAGE133
- FM_PSG_ZEROED_BALANCED_CLOCK_RQ
- balancedClock=YES
- noHardTrim=YES
- target=3072 max=16384 lowWater=1536
- latencyFrames= / latencyMs=
- trimEvents= / clockSleeps= / drops=
- NATIVE_AUDIO_STREAM_START_RQ_BALANCED_AUDIO_CLOCK_QT
- singleAudioPath=YES activeAudioTracks=1
