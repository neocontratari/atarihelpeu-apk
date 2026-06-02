AtariHelp.eu EMU-09 FIX86 GENERIC OSROM + ASYNC + DEATH RACE

Cil: nepokracovat jako jednoherni zaplaty, ale posilit obecne jadro pro XEX hry, ktere skacou do prazdne OS ROM nebo konci VBI/DLI pres ROM exit.

Zmeny:
- vychazi z FIX85, Cobra/Donkey/Galaxian smer ponechany, joystick ponechan;
- generic async OS exit guard pro VBI/DLI rutiny: $C000-$CFFF, $D000-$D7FF, $D800-$FFFF se v interrupt wrapperu bere jako OS exit, ne jako kod k provedeni;
- opravuje pripad Death Race, kde VBI skocil na $C0E2/$CFFF a mini-emulator pak vykonaval prazdnou ROM az do GTIA $D006;
- pridan vestaveny TEST DEATH RACE XEX;
- snapshot vypisuje ASYNC OS EXIT FIX86, aby bylo videt, jestli obecna ochrana zafungovala.

Test poradi:
1) Super Cobra - scroll, vrtulnik, kulomet;
2) Donkey Kong - start a hra;
3) Galaxian - jestli nezmizel dosavadni pokrok;
4) Death Race - jestli VBI trap na $CFFF/$D006 zmizel nebo se zmensil.
