AtariHelp.eu EMU-09 FIX127 STABLE REFS DLIST RECOVERY CORE

Co se mění:
- Cobra: chráněný hard-lock na ověřený runtime DLIST $2000; $462B/$B439 už nesmí vyhrát nad baseline.
- Arkanoid III: přednostně drží známou DLIST $3503, až pak živé přechodové pointery.
- Moon Patrol: $854C je v karanténě jako falešný DLIST; zkouší se $7481 a sken nesmí uložit $854C jako lastGood.
- Donkey: zůstává chráněný referenční režim.
- Montezuma PRELIM: nechána stabilnější cesta z posledních testů, bez agresivního ROM přepisu.
- Pac-Man: zůstává diagnostika, zatím bez agresivního zásahu.

Test:
1) Ověř build tag FIX127_STABLE_REFS_DLIST_RECOVERY_CORE.
2) Cobra: START, jestli se už nerozpadne.
3) Donkey: rychle ověřit, že běží.
4) Arkanoid III: hlavně menu a první obraz hry.
5) Moon Patrol: snapshot log, musí ukázat, že $854C už není použitý jako dobrý DLIST.
6) Montezuma PRELIM: potvrdit, že hra pořád běží a není horší po smrti/reloadu.
