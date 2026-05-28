EMU-09 FIX11 GENERIC DLIST

- Opraven pád při načtení cizího XEX: renderRegs is not defined.
- Vestavěný PiTT-KiTT-NaP-Final.xex vyměněn za aktuální soubor dodaný Reném.
- RESET + VESTAVĚNÝ PiTT-KiTT teď maže i výběr souboru a vrací se opravdu na interní XEX.
- Přidán obecnější ANTIC display-list renderer pro vlastní XEXy: umí číst cizí SDLST/DLIST a základně vykreslit mode 2/4/5 a fallback pixelové módy.
- Přidán pre-run INITAD sekvencí před hlavním spuštěním, protože některé XEXy nastavují obraz nebo rozbalují data přes INITAD.

Poznámka: pořád to není plná Atari emulace. XEXy s DLI/VBI, přesnou GTIA/ANTIC časovací logikou, ROM službami nebo vlastními packery budou chtít další doplnění.
