EMU-09 FIX3 / 4-player PMG car + real shadows

Základ: EMU09 FIX2, žádná V10 větev.

Opravy:
- PMG zůstává double-line: missiles $4180, P0 $4200, P1 $4280, P2 $4300, P3 $4380.
- Renderer teď sleduje, jestli hra zapsala shadow registry PCOLR/COLOR. Zapsaná $00 je platná barva.
- P2/P3 = černé pneumatiky, už se nepřebírá stará GTIA barva z intra.
- Auto je brané jako 4 hráči: P0+P2 levá půlka, P1+P3 pravá půlka.
- Přidán PMG debug render se 4 diagnostickými barvami.
- Odstraněny staré červené debug obdélníky/pruhy z obrazu.

Test:
Load XEX -> BOOT GAME / HOLD START -> RENDER GAME 2BIT RAM
Pak vyzkoušet:
- RENDER PMG ONLY / REAL COLORS + WHEEL HALO
- RENDER PMG ONLY / 4-PLAYER DEBUG COLORS
