AtariHelp.eu EMU-09 FIX95 DLIST STABILITY + SCAN CORE

Co je nové podle testu FIX94:
- SNAPSHOT už nespadne na chybě viewportDiagnosticInfo: FIX95 přidává bezpečnou diagnostickou funkci a další řádky DLIST stability.
- Donkey: stable-frame ochrana už nesmí uložit přechodový DLIST $1200 jako „stabilní“. Tím míříme přesně na rozsypaný čaj mezi menu/intry, bez zásahu do samotné hry.
- Cobra: když je aktuální SDLST/HW DLIST jen nízký menu/shadow list okolo $2000 a scanner najde lepší runtime DLIST, FIX95 ho povýší. Cíl je vrátit správnější obraz a horní HUD/text, ne jen maskovat snímek.
- Arkanoid III: vypnutý automatický Gentle Boot Assist pro Arkanoid. FIX94 ho po autostartu pravděpodobně přeskočil rovnou z menu do rozbité herní obrazovky. Ve FIX95 se startuje CPU, ale START/FIRE zůstává ruční přes TAP.
- Logy ukazují nové položky: VIEWPORT DIAG FIX95, DLIST STABILITY FIX95, cobraScanPromotions a arkanoidAssistBlocked.

Co chránit:
1. Donkey Kong: nesmí se zhoršit gameplay ani centrování. Sleduj hlavně šum mezi intry a drobné kousnutí ve hře.
2. Super Cobra: vrtulník/scroll nesmí zmizet. Sleduj menu a horní HUD/text.
3. Arkanoid III: po načtení počkej 3–5 sekund bez mačkání, jestli zůstane menu. Teprve potom TAP START/FIRE.
4. Pitstop/River/Montezuma/Death Race: jen krátká kontrola, jestli FIX95 strukturálně nerozbil další reference.

Commit Summary pro GitHub Desktop:
FIX95 DLIST stability scan core

Commit Description:
- Fix snapshot crash by adding viewportDiagnosticInfo diagnostics.
- Prevent Donkey/Cobra transition DLISTs from being remembered as stable frames.
- Promote better scanned Cobra runtime DLIST when low shadow/menu DLIST is selected.
- Disable automatic gentle boot input for Arkanoid so the menu is not skipped.
- Update build tag, labels, README and snapshot diagnostics to FIX95.

Test, který potřebuji:
A) Donkey Kong: nechat proběhnout menu/intra, pak gameplay 2 minuty. Screenshot šumu mezi intry + jeden SNAPSHOT LOG po přechodu.
B) Super Cobra: screenshot menu hned po startu, pak gameplay po 20 sekundách, hlavně horní HUD/text + SNAPSHOT LOG.
C) Arkanoid III: načíst a 3–5 sekund nic nemačkat. Screenshot první obrazovky + SNAPSHOT LOG. Pak zkus TAP START/FIRE a další screenshot/log.
D) Jeden rychlý kontrolní test: Pitstop II nebo Montezuma, stačí screenshot + SNAPSHOT LOG.
