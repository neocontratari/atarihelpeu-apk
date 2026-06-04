EMU-09 FIX126 PROTECTED REFERENCES + MOON DLIST CORE

Co se menilo:
- Cobra se vraci na chraneny runtime DLIST $2000. FIX125 dovolil $462B a Reneho test ukazal rozpad obrazu po START.
- Arkanoid uz neni v ROM_PROBE; je chranena reference LEGACY, aby se nerozbilo menu.
- Moon Patrol ma nove poradi DLIST vyberu: shadow/HW pokud neni podezrely, potom stabilni $7481, az potom scan. $854C je blokovany jako falesny drawable/code kandidat.
- Donkey zustava chraneny, Montezuma PRELIM/ALT/ORIG zatim bez agresivniho prepisu.
- Pac-Man zustava v ROM_PROBE diagnostice; cil je nejdriv vycistit DLIST/OS cestu u Moon/Pac bez rozbiti referenci.

Test:
1) Overit build tag FIX126_PROTECTED_REFERENCES_MOON_DLIST_CORE.
2) Donkey, Cobra, Arkanoid rychle potvrdit regresi/ne-regresi.
3) Moon Patrol: screenshot + snapshot log, hlavne jestli DLIST uz neni $854C, ale $7481 nebo jiny stabilni.
4) Pac-Man: screenshot + snapshot log.
5) Montezuma PRELIM: game over/reload, zda zustava lepsi nez FIX116/FIX123.
