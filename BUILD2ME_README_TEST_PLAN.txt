AtariHelp.eu EMU-10 BUILD2ME_DOC_FRAME_SNAPSHOT_COLLISION_SAFE

ZAKLAD:
- BUILD2LR + dokumentacni opravy MA/MB/MC/MD
- ne LT/LU/LV/LW/LY/LZ
- UI / loader / klavesnice / joystick / kazeta / CLOAD / CSAVE beze zmen
- KODY JSOU STEJNE

PROC BUILD2ME:
MD test konecne ukazal realny posun:
- Popeye: lepsi, barvy sedi vic, hra mene rozsypana, ale veci nejsou presne na miste a v uvodnim intru chybi horni text.
- Night Driver: predek auta se vratil / je videt, ale zustal horizontalni pruh.

ME proto nejde zpet na filtry. Opravuje dalsi nizsi vrstvu:
1) VBXE display frame snapshot
   - XDL/HR/SR/LR renderer cte snimek VBXE VRAM zachyceny na zacatku viditelneho frame.
   - Predtim renderer cetl pozdni VRAM az po dobehnuti CPU frame, tedy data mohla byt uz rozpracovana pro dalsi obraz.
   - Cil: Popeye objekty/vrstvy bliz spravnym mistum, mensi chaos; Night Driver pruh muze byt rozpracovany SR buffer.

2) Blitter collision podle fx1.26
   - BCB blitter mode 1-5 collision = DEST & BLT_COLLISION_MASK.
   - Segmentove masky patri overlay/raster kolizim, ne BCB blitteru.
   - Mode 6 HR collision testuje nibble & low collision mask.

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2ME_DOC_FRAME_SNAPSHOT_COLLISION_SAFE pripraven
VBXE BUILD VERIFY BUILD2ME
VBXE FRAME SNAPSHOT DOC BUILD2ME
VBXE FRAME SNAPSHOT BUILD2ME
VBXE BLITTER COLLISION DOC BUILD2ME
VBXE BLITTER FEATURE BUILD2ME

TEST PLAN:
1) Popeye VBXE
- intro horni text: je / neni
- hra proti MD: lepsi / stejna / horsi
- objekty/vrstvy: bliz spravne / porad mimo / horsi
- barvy: zustaly lepsi / zhorsily se

2) Night Driver
- pruh: pryc / stejny / horsi
- auto/predek: cele / urizle / horsi

3) W3D jen pokud mas cas
- stejny / lepsi / horsi

NEtestovat ted Decathlon ani River Raid, pokud nechces hlidat regresi.
