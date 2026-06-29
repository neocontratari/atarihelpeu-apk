BUILD2OZ_SEGA_MOBILE_SAFE_INPUT_OFFLINE_DIAG_STAGE64

TEST:
1) Nox portrait: over, ze zvuk zustal cisty jako 2OY/2OW.
2) Mobil landscape Sonic: over joystick, rychle opakovane skoky A/B/C a reakci krize.
3) Mobil zvuk: over, zda oddeleni A/B/C a stabilni playback pomohlo ozvene. Pokud ne, je to core/audio-vrstva problem.
4) Vypni internet a zkus Sega: log ma ukazat OFFLINE_CORE_MISSING, dokud nebude v assets/emu_sega lokalni clownmdemu.js.
5) Uloz LOG pres disketu.

V LOGU CHCI:
- BUILD2OZ_SEGA_MOBILE_SAFE_INPUT_OFFLINE_DIAG_STAGE64
- AUDIO_TUNE installed latencyHint=playback
- inputMode=mobileSafe
- OFFLINE_CORE_MISSING pokud testujes bez internetu
