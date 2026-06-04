AtariHelp.eu EMU-09
FIX133_STABLE_VIDEO_COLLISION_CORE

Commit summary:
FIX133 stable video collision core

Cíl buildu:
- Nepokračovat v honění jedné hry. Stabilizovat obecné jádro po testu FIX132.
- Vrátit příliš agresivní syntetické DLI pásy z FIX132, protože Arkanoid bliká a Montezuma má rozjetou grafiku.
- Zlepšit Cobra kolize střely s playfieldem a zachovat vzdušné cíle.
- Ukázat jasně PLAYER 1 overlay po život/scéna změně v Super Cobra.
- U Moon Patrol nepauzuje start; když DLIST běží s nulovými GTIA barvami a bez NMI, použít bezpečný framebuffer fallback místo prázdné DLIST šedi.

Změny:
1) Build label opraven všude na FIX133_STABLE_VIDEO_COLLISION_CORE.
2) Konzervativní DLI: když DLIST nemá $80 DLI bity, neběží už široký pásový sweep 24..192 jako ve FIX132; použije se jeden střední vzorek y64. Skutečné DLI řádky z DLISTu dál běží normálně.
3) Super Cobra:
   - rozšířený forward probe pro missile/playfield kolizi, aby kulomet lépe chytil pozemní cíle,
   - sticky MxPF latch na několik framů, aby čtení po HITCLR nevracelo hned nulu,
   - PLAYER 1 overlay po detekci life/scene změny.
4) Moon Patrol:
   - pokud běží přes DLIST, ale GTIA PF/BK jsou všechny nula a není VBI/DLI, renderer přejde do framebuffer fallbacku.
   - přidán blank-GTIA palette fallback, aby se nezobrazovala jen černá/šedá plocha.
5) Snapshot log doplněn o videoFallback=... a nové FIX133 tagy.

Test plan:
1. Po spuštění ověřit v logu/snapshotu build tag: FIX133_STABLE_VIDEO_COLLISION_CORE.
2. Super Cobra: pozemní cíle kulometem, vzdušné cíle, smrt a text PLAYER 1.
3. Moon Patrol: jestli běží a jestli je vidět něco víc než prázdná/šedá DLIST plocha; poslat snapshot.
4. Arkanoid III: menu, barvy/šipka, potom hra. Důležité: jestli přestalo barevné blikání z FIX132.
5. Montezuma PRELIM: jestli se vrátila stabilnější grafika proti FIX132.
6. Donkey Kong: ověřit běh hry i šum mezi intry; Donkey není nedotknutelný, ale je dobrý referenční test.
