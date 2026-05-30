EMU-09 FIX51 BLANK DLI + PMG CORE

Cíl: pokračovat směrem k obecnému Atari XEX emulátoru, ne k jedné vestavěné hře.

Opravy proti FIX50:
- Generic DLI scheduler už bere DLI bit i u blank instrukcí display listu ($F0, $80 atd.).
  To je důležité pro Pitstop II i další hry, které nastaví barvy/CHBASE ještě před první viditelnou řádkou.
- Log teď vypisuje DLI SCANLINES FIX51, takže je vidět, jestli se $80 bit v DLISTu opravdu obsloužil.
- DLI chain log ukazuje y-pozici: y15@$4169 místo jen adresy rutiny.
- Compatibility log přidává PMG INFO pro River/Donkey a další XEX, aby šlo zjistit PMBASE, single/double layout, HPOS a SIZE.
- Build tag sjednocen na FIX51_BLANK_DLI_PMG_CORE.

Co to ještě není:
- Není to Altirra. Pořád chybí přesné scanline časování, plné GTIA priority, HSCROL/VSCROL a kompletní POKEY/OS chování.
- U Donkey/River posílej prosím vždy screen + log, protože teď bude log ukazovat, jestli chybí playfield, PMG, DLI nebo špatný display list.
