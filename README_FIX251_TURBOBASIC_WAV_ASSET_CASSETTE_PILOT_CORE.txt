AtariHelp.eu EMU-09 FIX251_TURBOBASIC_WAV_ASSET_CASSETTE_PILOT_CORE

CIL:
- pridat TurboBasicXXL.wav primo do APK assets jako testovaci kazetovy WAV pilot,
- obejit problem Android/Nox file pickeru pro prvni kazetovy test,
- umet WAV poctive nacist/analyzovat jako RIFF/WAVE PCM, logovat delku, format a signal,
- pridat PLAY/STOP/REWIND a CLOAD + PLAY tlacitka,
- NEDAVAT zadny fake LOAD do RAM.

TurboBasicXXL.wav:
- velikost 33558928 bytes
- RIFF/WAVE PCM
- mono 16-bit 44100 Hz
- delka cca 380.49 s
- md5 0965696a8e82a8c961ba31fb49a9a57a

Test:
1. Spust EMU-09 a over BUILD FIX251.
2. Klikni INSERT BUILTIN TURBOBASICXXL.WAV.
3. Snapshot musi ukazat CASSETTE INSERT OK a WAV INFO.
4. Zkus PLAY/STOP/REWIND.
5. POWER XL/XE BASIC -> CLOAD + PLAY. Zatim cekej jen real prikaz a kazetovy stav, ne hotove nacteni programu.

KODY JSOU STEJNE.
