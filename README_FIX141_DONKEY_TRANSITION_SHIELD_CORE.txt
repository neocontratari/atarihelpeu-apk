AtariHelp.eu EMU-09 - FIX141_DONKEY_TRANSITION_SHIELD_CORE

Cíl buildu:
- Zamknout úspěch z FIX140: Montezuma PRELIM už funguje, na tenhle směr se nesmí šlápnout.
- Super Cobra zůstává chráněná hratelná reference.
- Donkey Kong: zkusit viditelně zmenšit čaj/šum mezi dupajícím intrem, menu a hrou.

Změny:
1) Donkey $1200 transition shield
   - DLIST $1200 je pro Donkey přechodový/noise stav.
   - FIX141 ho krátce maskuje posledním stabilním snímkem.
   - Oproti starším pokusům je štít časově omezený, aby nerozbil menu ani start hry.

2) Montezuma PRELIM lock
   - Runtime DLIST promotion z FIX140 zůstává.
   - Montezuma framehold/restore zůstává vypnutý, aby se nevracely staré rozbité grafiky.

3) Mobil/WebView
   - FIX139 log guard zůstává.
   - Nepřidávám další škrcení rychlosti, aby Donkey/Cobra nebyly horší.

Build tag:
FIX141_DONKEY_TRANSITION_SHIELD_CORE

Commit summary:
FIX141 donkey transition shield core

Krátký test plan:
1. Donkey Kong - hlavně přechody: dupající intro -> menu -> hra -> další intro/level. Sleduj, jestli ubyl čaj.
2. Montezuma PRELIM - jen potvrdit, že pořád funguje jako ve FIX140.
3. Super Cobra - jen potvrdit hratelnost.
4. Arkanoid III a Moon Patrol - jen velmi krátce, tady se v tomhle buildu nečeká zázrak.
