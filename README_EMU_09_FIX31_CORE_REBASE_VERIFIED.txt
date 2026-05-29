EMU-09 FIX31 CORE REBASE VERIFIED

- Vychází z FIX30, ne z FIX26.
- PiTT-KiTT zůstává referenční XEX.
- Oprava ANTIC mode $D: 4-color bitmap má 40 bytů na řádek, ne 20.
- Ověřovací build tag ve snapshotu: FIX31_CORE_REBASE_VERIFIED.
- KBCODE $D209 a CH $02FC teď používají TAP START/FIRE/UP/DOWN.
- Přidané TAP UP / TAP DOWN pro menu her jako Arkanoid.
- Blank ANTIC instrukce $70 se v tomto základu berou jako BLK8, ne jako grafický režim.
