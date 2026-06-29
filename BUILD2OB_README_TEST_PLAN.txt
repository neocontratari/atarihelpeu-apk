BUILD: BUILD2OB_SEGA_AUDIO_HARD_RESET_STAGE40

ZAKLAD:
- navazuje na BUILD2OA
- obraz, crop, reset, ROM picker a ovladac beze zmeny
- meni se jen audio lifecycle / ciste zavreni WebAudio mezi starty a mezi ROMkami

INSTALACE:
1. Rozbal ZIP pres koren projektu v GitHub Desktop vetvi, kde uz je BUILD2NZ + BUILD2OA.
2. Commit zprava: BUILD2OB Sega audio hard reset stage40.
3. Spust v Noxu / Androidu.

TEST A - prvni start aplikace:
1. Zavri aplikaci uplne z Androidu/Noxu.
2. Spust aplikaci znovu.
3. Otevri Sega.
4. Vyber Sonic nebo hru, kde hned poznas zvuk.
5. Poslouchej prvni hudbu / prstynky / intro aspon 30 sekund.
6. ULOZENE -> uloz LOG.

TEST B - druha hra bez shazeni aplikace:
1. Nech prvni hru bezet aspon 30 sekund.
2. Pres HRY/SBIRKA/CARTRIDGE vyber druhou ROM.
3. Appka musi ukazat delsi SEGA POWER OFF / AUDIO HARD RESET pauzu.
4. Druha hra musi nabehnout bez chrceni.
5. ULOZENE -> uloz LOG.

TEST C - reset stejne hry:
1. Ve hre stiskni RESET u cartridge.
2. Musi probehnout stejny power-off / power-on cyklus.
3. Po nabehnuti zkontroluj zvuk.
4. ULOZENE -> uloz LOG.

V LOGU HLEDEJ:
- AUDIO_TUNE installed latencyHint=playback preferredSampleRate=48000 audioHardReset=YES deferredKill=YES
- AUDIO_PRIME_START
- AUDIO_PRIME_DONE
- POWER_OFF_POST_SENT waitForAudioClose=900ms
- POWER_OFF_REQUEST
- AUDIO_CONTEXT_CLEANUP_DONE
- POWER_OFF_DONE_ACK
- MODULE_POSTRUN

CO POSLAT ZPET:
- LOG po testu A
- LOG po testu B
- LOG po testu C, pokud reset stale chrci
- kratce napsat: prvni hra OK/ne, druha hra OK/ne, reset OK/ne

DULEZITE:
- pokud obraz nebo ovladac zustane stejny a chrci jen zvuk, pokracujeme dal jen v audiu
- pokud se rozbije obraz/ovladac, vratime pouze index/clown wrapper rozdil proti BUILD2OA
