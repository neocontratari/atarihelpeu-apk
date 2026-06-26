AtariHelp.eu EMU-10 BUILD2KT_GTIA_COLOR_PRIORITY_COMPLEX_PASS

Co je zmenene:
- navazuje na KS, ne na KR/KQ/KP slepe latch/staging vetve.
- opravuje barvy komplexneji: normalni COLPM/COLPF/COLBK registry dal ignoruji luma bit0, ale GTIA9 pixel generator pouziva plny 4bit jas.
- GTIA11 ponechano podle dokumentace: hue=(COLBK hue OR pixel), luma=COLBK, pixel 0 ma nulovy jas.
- opraveno PRIOR bit2/bit1/bit0 pro PMG barevne vrstvy; bit2 uz neni spatne "vsechen PF nad PMG".
- bez screen-paintu, bez bitmapy, bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

Proc:
- Postcard porad ukazuje spatne barvy/pruhy.
- Decathlon a Donkey se po KS mirne pohnuly, takze zustavame u obecne GTIA/PMG cesty, ale opravujeme barvu a priority najednou.

Test:
1) Postcard_Atari_Rocky.xex: barvy/pruhy mensi/stejne/horsi.
2) The Activision Decathlon.xex: lepsi/stejne/horsi.
3) Donkey title/arcade pokud mas po ruce: lepsi/stejne/horsi + zvuk ano/ne.
4) Pokud stihnes: Commando jen kontrola Arnold ANO + hra ANO.

Log markery:
- BUILD2KT_GTIA_COLOR_PRIORITY_COMPLEX_PASS
- GTIA9 RAW LUMA BUILD2KT
- GTIA11 COLOR BUILD2KT
- GTIA PRIOR TABLE BUILD2KT
