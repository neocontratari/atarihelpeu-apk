EMU-09 FIX118 ROM UI SAFE DLIST CORE

Co je nového:
- Oprava a zviditelnění práce s volitelnou Atari OS ROM/BASIC ROM.
- Tlačítka ROM teď jasně píšou, že se vybírá .rom/.bin, ne .xex hra.
- Při výběru špatného souboru, hlavně XEX, emulator napíše srozumitelnou chybu místo matoucího chování.
- Po načtení ROM se hra sama nespouští: ROM je jen servisní podklad. Pak je potřeba znovu načíst/testovat XEX, aby běžel čistě s ROM aktivní.
- Snapshot log teď ukazuje zvlášť OS:on/OS:mini a BASIC:on/BASIC:mini.
- Donkey / Cobra / Arkanoid jsou dál chráněné reference.

Jak použít ROM:
1. Nejdřív běžně otestuj bez ROM.
2. Pokud chceš test s ROM, klikni NAČÍST OS ROM a vyber Atari OS ROM soubor .rom/.bin, ne XEX.
3. Volitelně klikni NAČÍST BASIC ROM a vyber BASIC ROM 8192 bytes.
4. Pak znovu klikni na TEST MOON PATROL / TEST PAC-MAN nebo NAČÍST VLASTNÍ XEX.
5. V logu musí být ROM=OS:on nebo BASIC:on. Když tam je OS:mini/BASIC:mini, ROM se nepoužívá.

Poznámka:
ROM soubory nejsou součástí ZIPu. Tento overlay jen umí načíst ROM, kterou už má tester legálně k dispozici.
