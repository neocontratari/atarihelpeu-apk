BUILD2LZ_LAST_CHANCE_VBXE_CORE_TRANSPARENT_SUPPORT_TILE_SAFE

ZAKLAD: BUILD2LR. LT/LU/LV/LW/LY nejsou zaklad.

CIL: posledni opravny rez bez game-patche podle nazvu hry.

ZMENY:
1) VBXE HR/SR/LR transparentnost: graficka nula respektuje VIDEO_CONTROL i pri priority=$FF.
2) Night Driver SR pruh: uz se neodreze cela radka; podezrela SR radka nechava jen pixely s vertikalni podporou v sousednich radkach.
3) Popeye tile/plosiny: realny BCB tile underlay se kresli do COLBK/cerneho pozadi pred HR/SR/LR overlay, aby ho transparentni nula mohla pustit ven.
4) UI, loader, CLOAD, CSAVE, klavesnice, joystick beze zmen.

LOG MUSI OBSAHOVAT:
AtariHelp.eu EMU-10 BUILD2LZ_LAST_CHANCE_VBXE_CORE_TRANSPARENT_SUPPORT_TILE_SAFE pripraven
VBXE GRAPH TRANSPARENT BUILD2LZ
VBXE SR SUPPORT FILTER BUILD2LZ
VBXE TILE UNDERLAY BUILD2LZ

TESTUJ JEN:
Night Driver: pruh pryc/stejny, auto cele/urizle.
Popeye VBXE: horni napis videt/chybi, schody/plosiny videt/chybi, chaos stejny/lepsi/horsi.
W3D: stejne/lepsi/horsi.
Decathlon ted netestovat.
