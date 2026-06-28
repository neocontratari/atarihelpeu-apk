BUILD: BUILD2OA_SEGA_AUDIO_CLEAN_DIRECT_STAGE39

TEST:
1. Rozbal ZIP pres koren projektu.
2. Spust APK/Nox.
3. Otevri Sega modul.
4. Vyber Sonic.
5. Testuj jen zvuk: prstynky, intro, gameplay.
6. Obraz/reset/ovladac se nema zmenit.
7. ULOZENE -> posli LOG.

V LOGU HLEDEJ:
- AUDIO_TUNE installed latencyHint=playback preferredSampleRate=48000 audioOnlyDirect=YES videoSyncOff=YES
- VIDEO_SYNC_DISABLED_AUDIO_ONLY direct canvas, no copy loop
- image tune zustava PLAY_CROP_TOP_NOISE_MICRO_NY
