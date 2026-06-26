AtariHelp.eu EMU-10 BUILD2LB_LOCKED_FAST_LOAD_STABLE_NO_GRAPH_GUESS_CORE

PRAVDA:
- Tohle neni dalsi graficky odhad.
- Zaklad je potvrzeny KZ/KV smer: Donkey Arcade a Donkey Junior zustavaji chranene.
- LA graficky palette-guard sem NEDAVAM, protoze nebyl otestovany a nechci rozbijet fungujici hry.
- XEX/OBX fake-fast je zesileny.
- ATR/D1 fake-fast je zesileny jen pro diskove SIO cteni.
- CLOAD/CSAVE/kazeta nejsou zrychlene.
- VBXE MAP-only XDL oprava z KZ zustava.
- Decathlon/Postcard/Night Driver nelakuju jako hotove.
- KODY JSOU STEJNE.

CO MUSI BYT V LOGU:
AtariHelp.eu EMU-10 BUILD2LB_LOCKED_FAST_LOAD_STABLE_NO_GRAPH_GUESS_CORE pripraven
XEX FAST LOAD BUILD2LB: locked fake-fast XEX/OBX nahravani zapnuto
ATR D1 FAST LOAD BUILD2LB: fake-fast ATR/D1 nahravani zapnuto jen pri diskovem SIO cteni
VBXE MAP-ONLY XDL BUILD2LB

TEST MINIMALNE:
1) Donkey Arcade: nahravani rychle, hra normalni rychlost, zvuk OK, clovicek/zebrik OK.
2) Donkey Junior: nahravani rychle, hra normalni rychlost, zvuk OK, opicka/zebrik OK.
3) W3D ATR: jestli ATR/D1 nahravani je rychlejsi nez KZ.
4) Popeye VBXE: jestli nahravani rychlejsi, grafiku zatim nelakovat.
5) Night Driver / Decathlon / Postcard: jen kontrola, zda se nerozbily vic.

KDYZ SE ROZBIJE DONKEY ARCADE/JUNIOR, LB zahodit.
KDYZ JE JEN GRAFIKA Decathlon/Postcard/Night porad spatne, je to o dalsi realne graficke vetvi, ne o loaderu.
