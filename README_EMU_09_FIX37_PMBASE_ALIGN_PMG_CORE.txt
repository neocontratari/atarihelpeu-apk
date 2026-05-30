EMU-09 FIX37 PMBASE ALIGN / PMG ENABLE CORE

- Vychází z FIX36.
- PiTT-KiTT zůstává referenční XEX beze změny.
- Oprava PMBASE: emulátor maskuje PMBASE jako Atari hardware.
  Single-line PMG = 2K hranice, double-line PMG = 1K hranice.
  Pitstop II zapisuje PMBASE $9B, skutečná báze je $9800.
- PMG overlay se kreslí jen když jej povolí GRACTL a DMACTL/SDMCTL.
  To má odstranit falešný modrý PMG sloup v Pitstop II po zadání jména.
- Keyboard/SETVBV/VBI opravy z FIX35/FIX36 zůstávají.
