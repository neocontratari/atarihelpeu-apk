EMU-09 FIX17 XEX AUTOSTART LAB

- PiTT-KiTT profil zůstává z FIX15/FIX16 chráněný a XEX soubor se nemění.
- Vestavěný River Raid a ručně načtené vlastní XEXy se po načtení automaticky spustí.
- Důvod: snapshot z FIX16 ukazoval PC=$4000, steps=0, tedy River byl jen načtený, ale CPU neběželo.
- Snapshot nyní vypíše playMode RUN/PAUSE a varování při steps=0.
