EMU-09 FIX20 RIVER INPUT/POKEY STATUS

- PiTT-KiTT zůstává referenční hra beze změny.
- River Raid: opraveno čtení GTIA TRIG1-TRIG3 ($D011-$D013) na klidovou hodnotu 1.
- River Raid: POKEY SKSTAT $D20F vrací klidově $FF.
- Opravená 6502 LSR rutina: shiftovala omylem dvakrát.
- River teď po pár snímcích začíná plnit obrazová data $2000/$3000 místo čisté vody.

Poznámka: River Raid stále není hotový jako v Altirře. Další krok bude DLI/PMG scanline renderer a správnější ANTIC/GTIA barevné změny po řádcích.
