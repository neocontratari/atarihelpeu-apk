AtariHelp.eu EMU-09 - FIX135_ADAPTIVE_STRATEGY_NO_FAKE_OVERLAY_CORE

Commit summary:
FIX135 adaptive strategy no fake overlay core

Hlavní změny:
- zachovává automatickou kompatibilní strategii z FIX134, ale opravuje stale DLI diagnostiku: DLI bity v DLISTu se berou jako aktivní jen když je aktivní NMI/DLI vector.
- Super Cobra: odstraněn umělý trvalý PLAYER 1 overlay. Emulátor už text nekreslí falešně, pouze loguje diagnostické okno po změně life/scene.
- Super Cobra: rozšířený MxPF latch pro kulomet/pozemní cíle. Pokud projektil opravdu narazí do solid playfieldu v herním okně, emulátor vrací širší PF masku, protože hra může číst různé MxPF bity.
- Moon Patrol: ponechán framebuffer fallback z FIX134, protože přinesl první viditelný posun.
- Build label přepsaný na FIX135 všude v aplikaci i snapshotu.

Test plan:
1) Ověřit build tag FIX135_ADAPTIVE_STRATEGY_NO_FAKE_OVERLAY_CORE.
2) Super Cobra: po smrti nesmí PLAYER 1 viset pořád; kulomet zkusit proti pozemním i vzdušným cílům.
3) Moon Patrol: ověřit, zda zůstává první viditelný posun místo šedé plochy.
4) Arkanoid III: menu + hra, hlavně zda se nezhoršilo blikání.
5) Montezuma PRELIM: ověřit rozjetou grafiku ve hře.
6) Donkey Kong: běh hry a šum mezi intry.
