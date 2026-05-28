EMU-09 FIX10 XEX LOADER V2

Základ: FIX9.

Opravy:
- XEX loader už bere RUNAD ($02E0) i INITAD ($02E2).
- Pokud RUNAD chybí, startuje z posledního INITAD, ne ze staré náhodně uložené hodnoty runAddress.
- RESET AKTIVNÍ XEX a RESET + VESTAVĚNÝ PiTT-KiTT teď čistí vstupy, CPU, RAM, loop a framebuffer.
- XEX diagnostika vypisuje ENTRY/RUNAD/INITAD a segmenty.
- JSR do prázdné ROM oblasti $A000-$BFFF/$E000-$FFFF je zatím stub, aby loader nespadl hned na prázdné ROM rutině.
- BRK v $A000-$BFFF hlásí srozumitelně, že XEX může potřebovat cartridge/BASIC ROM nebo dekompresní loader.

Poznámka:
Mini-emulátor zatím není plná Atari 800 emulace. Některé cizí XEXy budou chtít další CPU instrukce, ANTIC režimy, OS ROM, BASIC/cartridge oblast nebo přesnější GTIA/POKEY.

PiTT-KiTT:
Do assets/sources byl vložen poslední dodaný ASM zdroj, ale vestavěný XEX se bez aktuálně zkompilovaného .xex nepřekládá automaticky. Pro aktualizaci hry v APK je potřeba dodat hotový PiTT-KiTT .xex.
