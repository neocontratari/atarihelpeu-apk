EMU-09 FIX9 MINI XEX LOADER

Navazuje na FIX8.
- Zachovává rychlost, textové obrazovky, PMG auto, zvuk a ovládání pod obrazovkou.
- Přidává ruční načtení vlastního .XEX souboru z telefonu přes Android WebView file chooser.
- XEX parser po načtení bere RUNAD ze $02E0/$02E1, jinak první segment.
- Pro skóre je doplněný decimal mode ADC/SED/CLD, aby BCD rekordy v PiTT-KiTT fungovaly správněji.
- Status obrazovky teď ukazuje hint: po Game Over/Výhře stiskni START; pokud skóre překoná HI, skočí podpis tří písmen.

Poznámka: je to mini XEX emulator, ne plná emulace Atari OS/ANTIC/POKEY. Některé cizí XEXy můžou potřebovat další instrukce, OS rutiny nebo režimy ANTIC.
