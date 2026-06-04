# FIX134_AUTO_COMPAT_STRATEGY_CORE

Overlay pro AtariHelp.eu EMU-09.

Hlavní změna proti FIX133: FIX132 barevný zásah není plošně odstraněný. Přidaný je AUTO compatibility strategy manager, který volí DLI politiku podle runtime signálů:

- real-dlist-bits: používá skutečné DLI bity z display listu
- adaptive-banded: použije FIX132-style pásy u nechráněných titulů, kde runtime signály říkají, že to může pomoct
- soft-protected / soft-arkanoid-stable / soft-default: opatrný jednovzorkový režim pro hry, kde široké pásy blikaly nebo rozhazovaly grafiku

Zůstává: Moon FIX129 handoff rollback, blank-video fallback, Cobra sticky MxPF, PLAYER 1 overlay a opravený build label.

Build tag musí být: FIX134_AUTO_COMPAT_STRATEGY_CORE

Commit summary:
FIX134 auto compat strategy core

Test plan:
1. Ověřit tag FIX134 v aplikaci/snapshotu.
2. Super Cobra: kulomet zemní/vzdušné cíle, po smrti PLAYER 1.
3. Moon Patrol: běh + grafika/fallback.
4. Arkanoid III: menu, barvy, blikání, první hra.
5. Montezuma PRELIM: jestli se grafika nerozjede hůř.
6. Donkey Kong: šumy mezi intry + hratelnost.

V logu sleduj: AUTO COMPAT FIX134, dliMode a FIX134 CORE.
