# AtariHelp.eu EMU-09 FIX67 PITT TEXT + COBRA DLIST + CORE TXT

FIX67 reaguje na test z 2026-06-01:

- PiTT-KiTT Remaster: opraveno rozbite pismo. Textovy ANTIC radek se v pomalem rendereru kreslil znovu na kazdem scanline, cimz se pismena prekryla do bilych bloku. Ted se textovy radek kresli jednou za ANTIC textovou radku.
- Super Cobra: CPU bezel, ale obraz padal do nahodneho 2BPP sumu, protoze kratky DLIST s vice LMS radky byl odmitnut jako "malo viditelny". Obecna DLIST metrika ted prijme i kratke validni loader/menu DLISTy.
- CORE TEST TXT zustava automaticky ukladany do TXT souboru a po testu vraci puvodni stav emulatoru.
- PiTT-KiTT Remaster zustava chraneny referencni titul, neni soucast core matrix testovacich polozek.

Co testovat jako prvni:

1. RESET + VESTAVENY PiTT-KiTT - pismo musi byt citelne, zadne bile bloky.
2. Super Cobra - uz nema spadnout do barevneho nahodneho sumu; sleduj, jestli se objevi realny DLIST obraz/menu.
3. CORE TEST TXT - musi ulozit TXT report automaticky.
4. Pitstop II - poslat stav vrch/spodek obrazovky.
5. Vlastni XEX: Pac-Man a Donkey Kong.

GitHub Desktop summary:

FIX67 pitt text cobra dlist core txt
