AtariHelp.eu EMU-09 FIX145_DONKEY_STRICT_466F_SPEED_CORE

Commit summary:
FIX145 donkey strict 466f speed core

Co je v buildu:
- Zachovává rychlostní zisk z FIX144.
- Donkey Kong: žádný canvas framehold / žádné getImageData držení starého obrazu.
- Donkey Kong: $1200 a časný $49D7 jsou blokované jako přechodový čaj.
- Donkey Kong: čistý runtime DLIST $466F má prioritu, potom lastGood/scan, $49D7 až jako pozdní nouze.
- Montezuma PRELIM oprava z FIX140 zůstává zamčená.
- Super Cobra zůstává chráněná hratelná reference.

Test plan:
1) Donkey Kong: intro -> menu -> hra -> přechody, sleduj šum a rychlost.
2) Montezuma PRELIM: jen potvrdit, že pořád funguje.
3) Super Cobra: jen potvrdit hratelnost a rychlost.

V logu hledej:
BUILD TAG FIX145_DONKEY_STRICT_466F_SPEED_CORE
DONKEY FIX145
FIX145 strict $466F
