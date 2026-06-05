AtariHelp.eu EMU-09 FIX177 REALOS GTIA 9 10 11 RENDER CORE

Navazuje na FIX176.

Rene/Partak workflow:
- ZIP overlay rozbalit do lokalniho repo.
- Commit/push pres GitHub Desktop.
- GitHub Actions postavi APK.
- Test v Noxu/mobilu.

Co se meni:
- Klavesnici z FIX175/FIX176 nechavam funkcne beze zmeny.
- FIX177 opravuje renderer GTIA GRAPHICS 9/10/11 nad ANTIC mode F + PRIOR $40/$80/$C0.
- GRAPHICS 9: 16 luminanci z COLBK hue.
- GRAPHICS 10: 9 GTIA registru, doplneny viditelny fallback pro hodnoty 9-15.
- GRAPHICS 11: 16 hue, pri nulove luminanci v REAL OS testu pouzita ctelna luminance, aby nebyl obraz cerny.
- Pridana diagnostika GTIA RENDER FIX177.
- Pridana tlacitka GTIA 9 TEST / GTIA 10 TEST / GTIA 11 TEST pod BASIC TXT.

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. Rychle overit LET A=5, ? "AHOJ", PRINT 2>1.
3. GRAPHICS 7 referencne.
4. Stisknout GTIA 9 TEST, pockat az se dopise a napsat RUN, nebo test spustit z vlozeneho programu.
5. Stejne GTIA 10 TEST a GTIA 11 TEST.
6. Poslat screenshot a snapshot/log. Hledat radky GTIA MODE FIX177 a GTIA RENDER FIX177.

Commit summary:
FIX177 realos gtia 9 10 11 render core
