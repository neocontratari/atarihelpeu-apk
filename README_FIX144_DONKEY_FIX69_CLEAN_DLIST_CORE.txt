FIX144_DONKEY_FIX69_CLEAN_DLIST_CORE

Commit summary:
FIX144 donkey fix69 clean dlist core

Co je nové:
- Vzal jsem dodané APK „app-debug 9fix 69 donkey bez chyb.apk“ jako referenci pro Donkey Kong.
- FIX143 zmenšil šum, ale pořád příliš brzo padal na nouzový DLIST $49D7.
- FIX144 u Donkey preferuje live shadow/HW DLIST jako ve starším principu, při přechodu $1200 zkusí nejdřív starý čistý runtime DLIST $466F, potom silný runtime scanner, potom lastGood, a až nakonec $49D7.
- Canvas framehold/getImageData pro Donkey zůstává vypnutý, aby nebyli duchové ani zpomalení.
- Montezuma PRELIM oprava z FIX140 zůstává zamčená.
- Super Cobra zůstává chráněná hratelná reference.

Test plan:
1. Donkey Kong: intro -> menu -> hra -> další přechod. Sleduj šum a rychlost.
2. Montezuma PRELIM: jen potvrdit, že pořád funguje.
3. Super Cobra: jen potvrdit, že pořád hratelná.
4. Arkanoid III / Moon Patrol jen rychle, FIX144 je neřeší.

V logu hledej:
- BUILD TAG FIX144_DONKEY_FIX69_CLEAN_DLIST_CORE
- DONKEY FIX144 ... info=FIX144 FIX69 clean $466F ... nebo scan runtime ...
