AtariHelp.eu EMU-09 FIX106_TRIGGER_COLLISION_PMG_STABILITY_CORE

Cil: vratit smer zpet k jadru emulatoru: sdileny TRIG0 pulzni vstup, GTIA latch cisteni a opatrne PMG filtrovani.

Zmeny:
- Cobra: menu fallback zustava, ale FIRE uz nepodstrkuje joystick DOWN. Drzeni FIRE generuje symetricky TRIG0 pulz 2 frame LOW / 2 frame HIGH.
- Cobra: HITCLR cisti hlavne objektove MxPL/PxPL stale latche, PF kolize zustavaji do dalsiho renderu.
- Cobra: vlastni strely/bomby nevyrabeji MxPL self-hit latch proti vrtulniku; MxPF pro trefy do playfieldu zustava.
- Montezuma: PMG stale-row filtr se vypina v pozorovanem death/respawn loopu $2600-$26FF, aby po smrti/climb stavu nerozbil postavu v zebričku.
- Donkey a Arkanoid: chraneny stav beze zmen.

Test:
1) Cobra: menu, drzeni FIRE, pomale tapy FIRE, kolize s terenem/nadrzi, snapshot log.
2) Montezuma: nechat intro, hrat, zemrit, sledovat duchy a znovuobjeveni.
3) Donkey: potvrdit, ze bez zhorseni.
4) Arkanoid: menu OK.
