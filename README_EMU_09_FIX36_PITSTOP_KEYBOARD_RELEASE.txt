EMU-09 FIX36 Pitstop Keyboard Release / VBI Core

- PiTT-KiTT je referenční hra a XEX se nemění.
- Oprava Pitstop II: FIRE/START se po zadání jména už nesmí mapovat do CH $02FC jako klávesa T ($2D).
- OS keyboard stub vrací ATASCII jen pro skutečnou mobilní Atari klávesnici / frontu jména.
- Pokud není žádná klávesa, stub vrací bezpečný no-key stav, ne falešný znak.
