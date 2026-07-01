BUILD2QZ_SEGA_NATIVE_CPP_ONLY_CLEAN_TEXTURE_MOBILE_STAGE116

PROC TENTO BUILD:
Uz nejde o dalsi zalepeni jedne hry. QY dokazal, ze region respektuje ROM header a Samurai Europe uz bezi, ale S8 nestihal video. QZ presouva Sega C++ video z UI/WebView kresleni na TextureView render thread a zmensuje native frame copy overhead.

TEST NOX:
1) Aplikuj overlay pres BUILD2QY.
2) Spust appku -> SEGA MEGA DRIVE.
3) SBIRKA -> Sonic.
4) Odejdi Atari 130XE -> zpet Sega -> SBIRKA -> Sonic/Aladdin.
OCEKAVANI:
- Nesmi byt cerna obrazovka se zvukem.
- C++ CORE tlacitko nesmi byt funkcni ani jako DOM tlacitko.

TEST S8:
1) Spust Sega -> SBIRKA -> Sonic.
2) Test na vysku: sleduj plynulost obrazu.
3) Otoc landscape: obraz hry ma bezet pod pruhlednym ovladanim.
4) Vrat portrait: nesmi zustat cerna obrazovka.
OCEKAVANI:
- Video ma byt plynulejsi nez QY.
- Horni texty v landscape nemaji byt videt.
- C++ CORE nesmi byt klikaci.

LOG MARKERY:
- BUILD2QZ_SEGA_NATIVE_CPP_ONLY_CLEAN_TEXTURE_MOBILE_STAGE116
- NATIVE_TEXTURE_THREAD_START_QZ
- NATIVE_TEXTURE_FRAME_QZ
- NATIVE_TEXTURE_THREAD_STOP_QZ
- mobile video fast path
- NO_PATCH_RESPECT_ROM_HEADER

KDYZ TO BUDE PORAD POMALE:
Posli S8 log a napis, jestli se zlepsilo proti QY aspon trochu. Pokud QZ nepomuze, dalsi krok uz musi byt hlubsi audit core timing/render callbacku a pripadne presun na Surface/GL renderer, ne dalsi CSS/UI zaplata.
