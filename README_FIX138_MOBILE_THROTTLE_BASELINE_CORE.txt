AtariHelp.eu EMU-09
FIX138_MOBILE_THROTTLE_BASELINE_CORE

Cíl:
- stabilizovat po FIX136/FIX137 mobilní výkon bez dalšího vizuálního rizika
- držet FIX137 obrazový baseline: Super Cobra hratelná, Donkey zpět s menu/intrem, Moon/Arkanoid/Montezuma bez nových experimentů
- přidat automatický Android/WebView throttle, aby emulátor nejel jako PC-only test

Změny:
1) Mobile render throttle
- emulátor detekuje Android/Mobile/WebView/Nox userAgent
- u těžkých/nečitelných fallback profilů automaticky snižuje frekvenci renderování, ne CPU timing
- profilově opatrné: Super Cobra a Donkey nejsou uměle škrcené jako první volba

2) Žádný nový vizuální hack
- Moon Patrol zůstává na FIX137/FIX135 fallbacku, bez těžkého FIX136 LMS skenu
- Donkey transition suppress zůstává vypnutý
- Super Cobra fake PLAYER 1 overlay zůstává vypnutý

3) Diagnostika výkonu
- snapshot obsahuje MOBILE PERF FIX138
- status řádek ukazuje renderEvery a mobile guard stav
- cílem je, aby další buildy šly měřit bez ručního hádání, jestli se mobil dusí na renderu nebo CPU

Build tag:
FIX138_MOBILE_THROTTLE_BASELINE_CORE

Commit summary:
FIX138 mobile throttle baseline core

Test plan:
1. Ověřit build tag FIX138.
2. Mobil A12: krátce Super Cobra + Donkey, jestli je méně kousání.
3. Super Cobra: jen potvrdit, že zůstává hratelná.
4. Donkey Kong: menu/intro/hra, zda není horší než FIX137.
5. Moon Patrol: jeden snapshot kvůli MOBILE PERF FIX138 a stavu framebufferu.
6. Montezuma PRELIM: jen rychle, jestli není horší než FIX137.
