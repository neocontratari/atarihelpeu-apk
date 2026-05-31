EMU-09 FIX66 PITT SAFE CORE MATRIX TXT + 130XE

Summary pro GitHub Desktop:
FIX66 pitt safe core matrix txt 130xe

Co je dulezite:
- PiTT-KiTT Remaster je chraneny referencni titul.
- CORE TEST TXT nespousti PiTT-KiTT jako testovaci polozku.
- Po CORE TEST TXT se vrati puvodni stav emulatoru.
- Vysledek CORE TEST TXT se uklada automaticky do TXT souboru, bez rucniho kopirovani na mobilu.

Co testovat:
- RESET + VESTAVENY PiTT-KiTT: hlavni kontrola, ze referencni hra zustala funkcni.
- CORE TEST TXT: overit automaticke ulozeni TXT reportu.
- Super Cobra, Pitstop II, Arkanoid III, River Raid.
- Vlastni XEX: Pac-Man a Donkey Kong.

Princip smerem k Altirra:
Stavet obecne vrstvy emulatoru: CPU 6502, XL/XE memory map, PORTB bank switching, ANTIC display list/DMA, GTIA PMG/kolize/barvy, POKEY/PIA vstupy, OS vektory a interrupt timing. Nelepit jednu hru.
