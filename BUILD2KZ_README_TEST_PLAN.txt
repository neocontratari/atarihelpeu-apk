BUILD2KZ_FAST_LOAD_FAKE_OK_AND_VBXE_MAP_OFF_CORE

Co je zmenene:
- Navazuje na BUILD2KY.
- XEX/OBX nahravani je zamerne fake-fast, protoze uzivatel povolil fake pouze pro rychlost nahravani.
- CLOAD/CSAVE/kazeta se timto buildem nezrychluje a zustava mimo tuto vetev.
- KY record-aware turbo zustava, ale maxFast je zvyseny 28 -> 96, aby W3D/Popeye/VBXE/XEX nahravani nebylo zbytecne polovicni.
- VBXE MAP-only XDL oprava: kdyz XDL nema SR/LR/HR/TEXT overlay, ale ma MAPON/MAPADR, aplikujeme atributovou mapu a nepadame do surface fallbacku. Cil: rrumble / nektera VBXE intra s magenta/rozhozenym obrazem.
- Donkey Arcade / Donkey Junior PMG faze z KV/KY zustava.
- GTIA color-clock + HBLANK guard z KX/KY zustava.
- Postcard / Decathlon / Night Driver grafiku neprohlasuji za opravenou.
- Bez screen-paintu, bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

V LOGu ma byt:
AtariHelp.eu EMU-10 BUILD2KZ_FAST_LOAD_FAKE_OK_AND_VBXE_MAP_OFF_CORE pripraven
XEX FAST LOAD BUILD2KZ: fake-fast XEX/OBX nahravani zapnuto
XEX FAST LOAD BUILD2KZ: fake-fast record-aware XEX/OBX nahravani drzi az do posledniho XEX datoveho zaznamu
VBXE MAP-ONLY XDL BUILD2KZ

Test:
1) rrumble.xex - nahravani rychlejsi/stejne, obraz lepsi/stejny/horsi.
2) W3D - cas do intra a cas do hry.
3) Popeye VBXE - nahravani rychlejsi/stejne, grafika OK/spatna.
4) Night Driver - auto ano/ne, pruh ano/ne, zvuk ano/ne.
5) Decathlon - telo/nohy, napis nahore, rychlost.
6) Donkey Arcade - nesmi se rozbit clovicek/zebrik, zvuk, rychlost.
7) Donkey Junior - nesmi se rozbit opicka/zebrik, zvuk, rychlost.

Posli jeden spolecny LOG po sade.
