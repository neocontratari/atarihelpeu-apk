AtariHelp.eu EMU-09 FIX115 MONTEZUMA/MOON IDLE CORE

Co se menilo:
- Pridana treti vestavena Montezuma ALT z nahrane hratelnejsi verze.
- Montezuma: pri death/reload a zmene DLIST se cisti jen emulacni cache (framehold/DLI scan/PMG last-mid), ne RAM hry.
- Montezuma: START/FIRE gate uz nepousti vzdy kombinaci START+FIRE; pusti jen tlacitko, ktere tester opravdu zmackl.
- Moon Patrol: pokud loader spadne do OS/BRK, jadro zaparkuje hlavni CPU do male idle smycky a necha bezet VVBLKD/VBI, misto aby porad skakalo do spatneho entry nebo dat.
- Pac-Man: ponechana diagnostika BASIC/OS ROM dependency, protoze log stale ukazuje masivni BASIC/OS volani.
- Donkey/Cobra/Arkanoid: bez cileneho zasahu, aby zustaly baseline OK.
