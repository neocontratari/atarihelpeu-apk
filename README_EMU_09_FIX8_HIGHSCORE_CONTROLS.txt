EMU-09 FIX8 HIGHSCORE CONTROLS

Navazuje na FIX7 INTRO TEXT.

Změny:
- Atari ovládání přesunuto přímo pod canvas/obrazovku.
- Přidán směr joystick UP pro high-score zadávání iniciál.
- DOWN/SKOK zůstává na PORTA bit $02: ve hře skok, v high-score písmeno dolů.
- FIRE nebo RIGHT potvrzuje písmeno u rekordu.
- Testovací 1 SNÍMEK přesunut do Servisního debug panelu.
- Hlavní obrazovka je čistší: spustit, pauza, reset, zvuk. Výkonové volby jsou schované v detailu.

Základ zůstává EMU09/FIX7: double-line PMG, 4-player auto, intro text renderer, rychlý neblokující běh a první WebAudio/POKEY vrstva.
