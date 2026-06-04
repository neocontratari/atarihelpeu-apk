AtariHelp.eu EMU-09 FIX137 SAFE ROLLBACK PERFORMANCE CORE

Tento overlay je bezpečný rollback po neúspěšném FIX136.

Co se mění:
1. Vrací se rychlejší a stabilnější FIX135 video základ.
2. Vypnuté jsou těžké FIX136 změny: chytrý LMS framebuffer scan a dlouhé Donkey transition suppress. Ty na mobilu A12 zpomalily běh a rozbily Donkey menu.
3. Zůstává lehké čištění runtime stavu při novém XEX, ale pouze na load/reset, ne během hry.
4. Super Cobra zůstává na funkčním směru z FIX135: žádný falešný PLAYER 1 overlay, Cobra se bere jako referenčně hratelná.
5. Moon Patrol fallback zůstává konzervativní; nehoníme ho teď za cenu výkonu a regresí.

Commit summary:
FIX137 safe rollback performance core

Test plan:
1. Ověř build tag FIX137_SAFE_ROLLBACK_PERFORMANCE_CORE.
2. Na mobilu A12 otestuj rychlost: Donkey Kong a Super Cobra aspoň 1 minutu.
3. Super Cobra jen potvrdit, že je stále hratelná bez šumu.
4. Donkey Kong: menu/intra a jestli se opravilo rozbité menu z FIX136.
5. Moon Patrol: snapshot, ale nečekat zázrak; hlavní je, že se nezhorší výkon.
6. Arkanoid III a Montezuma PRELIM: jen rychlé ověření, jestli nejsou horší než FIX135.

Poznámka pro další vývoj:
FIX136 potvrdil dvě věci: izolace stavu je potřeba, ale drahý framebuffer/LMS scan nesmí běžet takhle plošně. Další skutečný pokrok má jít přes větší strukturální zásah: plánovač ANTIC/DLI/VBI po scanline a omezení render práce na mobilu, ne další kosmetická oprava jedné hry.
