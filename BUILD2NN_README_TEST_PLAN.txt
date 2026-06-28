BUILD2NN_SEGA_AUDIO_CROP_TUNE_STAGE27 - TEST PLAN

1. Rozbal overlay pres koren projektu.
2. Otevri Sega modul.
3. Vyber Sonic pres HRY / SBIRKA / CARTRIDGE SLOT.
4. Over, ze reset stale funguje jako v BUILD2NM.
5. Over obraz v monitoru: hra by mela byt jemne posunuta nahoru, pokud byla predtim malinko nizko.
6. Poslechni zvuk: mel by byt mene chraplavy, pokud WebView/Nox dovoli AudioContext playback/48k.
7. Zkus jinou hru a opet reset.
8. Zkus landscape.
9. ULOZENE -> posli LOG.

OCEKAVANE LOG MARKERY:
- BUILD2NN HARDBOOT AUTOBOOT
- WRAPPER AUDIO_TUNE installed latencyHint=playback preferredSampleRate=48000
- WRAPPER AUDIO_TUNE AudioContext sampleRate=... latencyHint=playback
- image tune: PLAY_CROP top=-10.7% height=123% landscapeTop=-9.5vh height=118vh
