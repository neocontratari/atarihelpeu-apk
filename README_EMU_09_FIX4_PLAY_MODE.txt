EMU-09 FIX4 PLAY MODE

Základ: EMU-09 FIX3 4-player PMG shadows.

Co se změnilo:
- odstraněný hlavní testovací les tlačítek z obrazovky emulatoru
- hlavní obrazovka má SPUSTIT EMULÁTOR / PAUZA / RESET / 1 SNÍMEK
- servisní debug je schovaný v rozbalovací sekci
- přidaná herní requestAnimationFrame smyčka CPU + render
- během hraní se do canvasu nepíše růžový debug overlay
- ovládání: START, FIRE, LEFT, RIGHT, SKOK
- SKOK je PORTA bit $02 podle ASM JUMP_LOGIC
- zachováno double-line PMG: missiles $4180, P0 $4200, P1 $4280, P2 $4300, P3 $4380
- zachováno 4-player auto: P0/P2 levá půlka, P1/P3 pravá půlka
