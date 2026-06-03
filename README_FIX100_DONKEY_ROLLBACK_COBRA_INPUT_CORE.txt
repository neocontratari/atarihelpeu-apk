AtariHelp.eu EMU-09 FIX100 DONKEY ROLLBACK + COBRA INPUT CORE

Cíl FIX100:
- Donkey: vrací příliš dlouhý FIX99 hold $1200 na krátký adaptivní hold, aby se šum nepřetahoval a hra nepřišla o skutečné přechodové snímky.
- Cobra: ponechává oddělení FIRE/DOWN a přidává úvodní grace masku proti dvěma nechtěným bombám po startu.
- Cobra HUD: přidává opatrný fallback pro horní GRAPHICS 0 text přes mini screen-code font, pouze pro horní HUD řádky.
- Montezuma: ponechává VVBLKD fallback a PMG kalibraci, protože človíček už je vidět.
- Arkanoid: ponechává čisté menu, hra samotná je další samostatná fáze.

Test priority:
1. Donkey menu/intro/před hrou/přechod do level 2 - hlásit, jestli je šum kratší než FIX99 nebo zpět jako FIX98.
2. Cobra - FIRE na startu, jestli zmizely první dvě bomby; horní HUD text; snapshot log.
3. Montezuma - človíček/duchy jen kontrolně.
4. Arkanoid menu kontrolně.
