AtariHelp.eu EMU-09
FIX147_MODE4_BOOT_VISUAL_CORE

Commit summary:
FIX147 mode4 boot visual core

Cíl buildu:
- Neřešit už dávno vyřešené tečky v textu.
- Zaměřit se na obecnější obrazový problém: nesmyslné písmo / rozhozené char režimy při startu a špatné vykreslování ANTIC mode 4/5 u více her.
- Zachovat rychlostní zisk z FIX144/FIX145/FIX146.
- Nerozbít Montezuma PRELIM a Super Cobru.

Změny:
1. ANTIC mode 4/5 128-glyph cesta
   - Vypnutá stará falešná 64-glyph/color-attribute interpretace pro mode 4/5.
   - Mode 4/5 teď obecně používá 128 znaků z CHBASE, jako běžnější Atari charset cesta.
   - Cíl: Donkey Kong Junior Title Version, Arkanoid-class obrazovky a další XEXy, kde byly znaky/intro rozházené.

2. Boot visual gate
   - Krátké startovní/loaderové okno už nemusí kreslit nesmyslný text z RAM/OS-ROM charsetu.
   - Místo canvas frameholdu se použije jen černý transient frame.
   - Žádné getImageData/putImageData držení starého obrazu, takže by se neměla vrátit brzda/duchové.

3. Live render cadence
   - Vypnuté profilové renderEvery 2..5 pro Arkanoid/Moon/Montezuma/generic.
   - Nox i mobil tak uvidí živější obraz; rychlostní zisk z odstranění canvas frameholdu zůstává hlavní optimalizace.

4. Chráněné reference
   - Donkey Kong zůstává na FIX69/$466F rychlé cestě.
   - Montezuma PRELIM oprava z FIX140 zůstává.
   - Super Cobra zůstává hratelná reference.

Build tag v aplikaci a snapshotu:
FIX147_MODE4_BOOT_VISUAL_CORE

Co testovat:
1. Donkey Kong original
   - rychlost musí zůstat jako FIX145/FIX146
   - sledovat šum mezi intry, ale hlavně nesmí být horší

2. Donkey Kong Arcade.xex
   - jestli se intro/hra méně míchá do smyčky
   - jestli je hra pořád rozsekaná nebo čitelnější

3. Donkey Kong Junior Title Version
   - intro, menu, začátek hry
   - hlavní očekávání: změna v rozhozených znacích / mode4 obrazu

4. Arkanoid III
   - menu a první hra
   - ověřit, jestli vypnutí renderEvery=5 zlepší blikání/živost obrazu

5. Montezuma PRELIM
   - musí dál fungovat

6. Super Cobra
   - musí dál zůstat hratelná

V logu hledej:
BUILD TAG FIX147_MODE4_BOOT_VISUAL_CORE
FIX147 CORE
bootVisual=
mode45=
MOBILE PERF FIX147
