BUILD2OD_SEGA_AUDIO_CLEAN_STABLE_STAGE42

CIL:
- vratit se k cistemu stabilnimu zvuku bez chrceni
- zachovat hard reset mezi hrami
- nehonit nizkou latenci za cenu praskani/chrceni

TEST PLAN:
1) Uplne zavri aplikaci.
2) Spust appku.
3) Sega -> vyber hru se zretelnym zvukem (napr. Aladdin, Sonic).
4) Over, zda je zvuk cisty a bez chrceni po startu.
5) Zahraj kratce 20-30 sekund.
6) Bez zavreni appky vyber druhou ROM.
7) Musi probehnout power-off mezikrok a nova hra se spustit s cistym zvukem.
8) Pokud bude zvuk cisty, teprve potom hodnot prodlevu jako druhoradou vec.
9) U her s hlaskou typu PAL / French SECAM posli screenshot nebo log - to je pravdepodobne regionova kompatibilita hry, ne primarne audio chyba.

V LOGU CHCI VIDET:
- BUILD2OD_SEGA_AUDIO_CLEAN_STABLE_STAGE42
- AUDIO_TUNE installed latencyHint=playback
- AUDIO_TUNE AudioContext sampleRate=48000 latencyHint=playback
- AUDIO_PRIME_START ... baseLatency=...
- POWER_OFF_DONE_ACK
- MODULE_POSTRUN

CO MI MAS POSLAT ZPET:
- prvni hra: cisty zvuk ano/ne
- druha hra bez restartu appky: cisty zvuk ano/ne
- prodleva: snesitelna / moc velka
- jmena her, ktere hodi PAL/SECAM hlasku
- log TXT a kdyz bude potreba screenshot
