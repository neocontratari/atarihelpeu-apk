AtariHelp.eu EMU-09
FIX132_UNIVERSAL_DLI_COLLISION_LABEL_CORE

Commit summary:
FIX132 universal DLI collision label core

Co se mění:
1) Opravené build názvy v aplikaci, indexu, snapshotu a logu. V logu musí být FIX132_UNIVERSAL_DLI_COLLISION_LABEL_CORE.
2) Moon Patrol: reálný rollback předchozího large-segment handoffu. Priorita je zpět na bezpečnější FIX129-style runtime code window okolo $9Dxx.
3) Obecné ANTIC/DLI: když hra má DLI vector, ale display list nemá jasné $80 DLI bity, emulátor sejme několik bezpečných vertikálních pásem. Cíl: lepší barvy textu/menu u více XEX, ne jen jedna hra.
4) ANTIC 6/7 text: lepší fallback pro barevné atributy, aby PF2/PF3 nezmizely na černém pozadí.
5) Atari screen-code znaky < > = doplněné pro menu kurzory/šipky.
6) Super Cobra: pozdní MxPF latch pro missile-playfield čtení, aby kulomet viděl i zemní cíle z posledního stabilního renderu.

Test plan pro Reného:
- Zkontroluj nahoře/log: FIX132_UNIVERSAL_DLI_COLLISION_LABEL_CORE.
- Super Cobra: kulomet proti zemním cílům i vzdušným cílům, pak snapshot při střelbě.
- Moon Patrol: zda už není okamžitý zásek; snapshot i když bude grafika špatně.
- Arkanoid III: menu barvy/šipka a první rozbitá hra; snapshot v menu i ve hře.
- Donkey Kong: jen ověřit, že hra běží a intro se nezhoršilo.
- Montezuma PRELIM: ověřit, že není horší než předchozí build.
