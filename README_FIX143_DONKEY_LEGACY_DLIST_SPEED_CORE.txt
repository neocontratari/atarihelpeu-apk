AtariHelp.eu EMU-09
FIX143_DONKEY_LEGACY_DLIST_SPEED_CORE

Účel buildu:
- Opravit špatný směr FIX141/FIX142 u Donkey Kongu bez dalšího lepení starým snímkem.
- Vrátit Donkey na lehčí legacy-live cestu podobnou starším buildům: žádný canvas frame-hold, žádné getImageData/putImageData držení duchů.
- Přechodový Donkey DLIST $1200 se nesmí ukládat jako stabilní a pokud se objeví, emulátor se pokusí kreslit živý/poslední stabilní runtime DLIST místo čaje.
- Zvýšit Donkey CPU budget na svižnější běh, protože už se neplatí cena canvas frameholdu.
- Zachovat Montezuma PRELIM opravu z FIX140.
- Zachovat Super Cobru jako hratelnou chráněnou referenci.

Změny:
1. Donkey legacy DLIST path:
   - nový Donkey branch v getDlistPtr()
   - shadow/hardware DLIST se používá jen když není přechodový $1200
   - $1200 se nahrazuje lastGood runtime DLISTem nebo ověřeným $49D7

2. Donkey bez canvas frameholdu:
   - Donkey už nevolá stable-frame capture přes getImageData
   - Donkey už nevrací starý snímek přes putImageData
   - odstraněné držení staré obrazovky, které přidávalo duchy, šum a zpomalení

3. Donkey rychlost:
   - Donkey CPU budget zvýšen na 5200 kroků / 8 ms
   - Super Cobra a Montezuma zůstávají na chráněném taktu

4. Diagnostika:
   - snapshot obsahuje DONKEY FIX143
   - core řádek obsahuje dk143=...

Build tag:
FIX143_DONKEY_LEGACY_DLIST_SPEED_CORE

Commit summary:
FIX143 donkey legacy dlist speed core

Test plan:
1. Donkey Kong — menu, dupající intro, šum mezi intry a rychlost hry. Hlavně ověřit, že není pomalejší než FIX140/FIX142 a že FIX141 regrese zmizela.
2. Montezuma PRELIM — jen potvrdit, že pořád funguje.
3. Super Cobra — jen potvrdit, že je hratelná.
4. Arkanoid III / Moon Patrol — jen rychle, tento build je neřeší.
