BUILD2OC_SEGA_AUDIO_LOW_LATENCY_STAGE41 - TEST PLAN

PROC TENHLE BUILD:
BUILD2OB zlepsil chrceni, ale uzivatel hlasi asi 1s opozdeni zvuku pri seknuti mecem / sebrani prstynku. V logu BUILD2OB byl AudioContext v rezimu latencyHint=playback. Ten je stabilni, ale muze mit velky buffer. BUILD2OC zkousi interactive low-latency audio, ale nechava hard reset mezi hrami.

TEST 1 - PRVNI START APLIKACE:
1. Aplikaci uplne zavrit.
2. Spustit aplikaci.
3. Sega -> vybrat ROM, kde je jasny okamzity zvuk: Sonic prstynek / Aladdin mec.
4. Otestovat reakci zvuku na akci.

CEKANE:
- Zvuk je porad cisty, bez stareho chrceni.
- Sek mecem / cinknuti prstynku nema byt opozdene o cca 1s.
- Mala prodleva v ramci emulace/WebView muze byt, ale nesmi to pusobit jako sekundovy buffer.

TEST 2 - DRUHA ROM BEZ ZAVRENI APK:
1. Bez zavreni aplikace vybrat druhou ROM.
2. Nechat probehnout SEGA POWER OFF / LOW LATENCY AUDIO RESET.
3. Otestovat zvuk znovu.

CEKANE:
- Nevrati se chrceni po druhe hre.
- Latence zustane mensi nez v BUILD2OB.

LOG MARKERY KONTROLOVAT:
- BUILD2OC_SEGA_AUDIO_LOW_LATENCY_STAGE41
- AUDIO_TUNE installed latencyHint=interactive
- AUDIO_TUNE AudioContext sampleRate=48000 latencyHint=interactive baseLatency=... outputLatency=...
- AUDIO_PRIME_START ... baseLatency=... outputLatency=...
- AUDIO_PRIME_DONE ok
- POWER_OFF_REQUEST
- AUDIO_CONTEXT_CLEANUP_DONE
- POWER_OFF_DONE_ACK
- MODULE_POSTRUN

CO POSLAT ZPET:
- LOG po prvni hre.
- LOG po druhe hre.
- Kratce: prvni hra cista/chrci, druha hra cista/chrci, latence lepsi/stejna/horsi.
