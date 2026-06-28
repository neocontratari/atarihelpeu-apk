BUILD2NS_SEGA_AUDIO_VISIBLE_CANVAS_SYNC_SAFE_STAGE32

TEST PLAN:
1) Rozbal ZIP pres koren projektu.
2) Spust APK/Nox.
3) Otevri SEGA modul.
4) Vyber Sonic pres HRY/SBIRKA/CARTRIDGE.
5) Kontrola A: musi byt videt obraz hry, ne cerna obrazovka.
6) Kontrola B: zvuk ma byt cisty jako v BUILD2NN/BUILD2NR.
7) Kontrola C: ovladac zustava funkcni.
8) Kontrola D: reset a vymena hry zustavaji funkcni bez zpomalovani.
9) Vyzkousej Aladdin - kontrola monitoroveho usazeni.
10) ULOZENE -> poslat log.

HLAVNI LOG MARKERY:
- VIDEO_SYNC_DIRECT_CANVAS mode=visible_real_canvas no_mirror_delay
- AUDIO_TUNE installed latencyHint=playback preferredSampleRate=48000
- WRAPPER CLOWN_SCRIPT_LOADED
- WRAPPER MODULE_POSTRUN

KODY JSOU ZMENENE.
