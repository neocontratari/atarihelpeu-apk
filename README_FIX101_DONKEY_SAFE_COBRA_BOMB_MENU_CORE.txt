AtariHelp.eu EMU-09 FIX101 DONKEY SAFE + COBRA BOMB/MENU CORE

Cíl FIX101:
- Nezhoršovat Donkeyho: vracíme se k bezpečnému krátkému přechodu, žádné dlouhé držení šumu z FIX99.
- Cobra: ruší se špatná představa, že bomba je chyba. U Super Cobry se po každé druhé střele vytvoří krátký DOWN/bomb puls, aby se port choval blíž originálu.
- Cobra menu/HUD: textový fallback se rozšiřuje z horního HUD na všechny GRAPHICS 0 textové řádky profilu Cobra, bez agresivního DLIST promotion.
- Montezuma: ponechaný VVBLKD fallback a PMG offset z FIX98/FIX100, protože človíček už je vidět.
- Arkanoid: ponechané čisté menu; rozbitá hra je samostatná další fáze.

Instalace pro Reného:
1. ZIP rozbalit.
2. Obsah složky zkopírovat přes repozitář v GitHub Desktop.
3. Commitnout.
4. Po GitHub Actions stáhnout nové APK.

Test:
1. Donkey Kong: jen ověřit, že se nevrátil dlouhý šum z FIX99 a hra běží.
2. Super Cobra: zkusit několik FIRE tapů. Po každé druhé střele má jít bomba. Udělat screenshot menu/HUD a snapshot log.
3. Montezuma: potvrdit, že človíček zůstal a zda jsou duchy stejné/lepší/horší.
4. Arkanoid: potvrdit čisté menu.
