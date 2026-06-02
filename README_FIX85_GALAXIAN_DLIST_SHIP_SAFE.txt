AtariHelp.eu EMU-09 FIX85 GALAXIAN DLIST + SHIP SAFE

Základ: FIX84 stabilní větev, Cobra/Donkey nechány bez zásahu.

Změny:
- Galaxian: uvolněný výběr naskenovaného runtime DLISTu, když je aktuální DLIST moc krátký.
- Galaxian: lokální low-memory PMG fallback pro player ship, když PMBASE=$0000 a HPOS registr zůstane 0.
- Galaxian: fallback je oříznutý jen na spodní část, aby se z PMG bufferu nestal svislý šum.
- Joystick: ponechána citlivost z FIX84.
- Cobra/Donkey/PiTT/Pitstop/KiTT: žádná záměrná změna jádra.

Testuj: Cobra, Donkey, joystick, Galaxian.
