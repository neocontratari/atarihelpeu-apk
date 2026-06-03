AtariHelp.eu EMU-09 FIX94 STABLE FRAME + INPUT/POT CORE

Co je nové:
- Donkey a další přechodové hry: místo pouhého nekreslení během změny DLISTu se drží poslední stabilní framebuffer. Cíl je zmenšit rozsypaný čaj mezi intry/menu.
- Frame hold už má diagnostiku: SNAPSHOT ukáže stable frame info, počet holdů a počet skutečných restore zásahů.
- Vstup se obnovuje každý frame jako Atari OS shadow STICK0/STRIG0/CH, ne jen při dotyku. To je společná oprava pro menu XEXů.
- Přidán jednoduchý POKEY POT0/POT1 + POTGO stub pro paddle hry, hlavně Arkanoid III. Joystick vlevo/vpravo mění POT0.
- Kruhový joystick zůstává chráněný pro Donkey/Cobra/PiTT/Pitstop/River/Galaxian; CH bridge pro joystick UP/DOWN je omezený na Arkanoid/generic menu.
- Index/labely byly uklizené z viditelného FIX77/FIX93 na FIX94, aby René netestoval starý APK omylem.

Co chránit:
1. Donkey Kong: obraz vycentrovaný, joystick dobrý, hra nesmí být horší. Sleduj šum mezi menu/intry a občasné cuknutí.
2. Super Cobra: scroll a vrtulník nesmí zmizet, horní HUD/text nesmí být horší.
3. Arkanoid III: zkus menu UP/DOWN a potom vlevo/vpravo jako paddle/POT.
4. Montezuma/Death Race: kontrolně, hlavně snapshoty.

Commit Summary pro GitHub Desktop:
FIX94 stable frame hold + input/POT core

Commit Description:
- Add stable framebuffer restore during DLIST transition holds for Donkey/generic/Cobra/Galaxian/Montezuma/Arkanoid.
- Refresh Atari input shadows every frame and add simple POT0/POTGO paddle stub.
- Add Arkanoid/generic UP/DOWN CH bridge while preserving protected reference profiles.
- Update build tag, labels, README and snapshot diagnostics to FIX94.

Test, který potřebuji:
A) Donkey Kong: spusť, nech proběhnout menu/intra, potom hraj 2 minuty. Pošli screenshot šumu mezi intry, screenshot z gameplay a jeden SNAPSHOT LOG po případném cuknutí.
B) Super Cobra: spusť, 20 sekund nech scroll, drž FIRE 5 sekund, pošli screenshot horního HUD/textu + SNAPSHOT LOG.
C) Arkanoid III: zkus menu nahoru/dolů přes kruhový joystick i tlačítka UP/DOWN, potom zkus vlevo/vpravo. Pošli screenshot menu a SNAPSHOT LOG.
D) Jedna další hra podle nálady: Montezuma nebo Death Race. Stačí screenshot + SNAPSHOT LOG po START/AUTO.
