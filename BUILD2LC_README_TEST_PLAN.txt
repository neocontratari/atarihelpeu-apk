AtariHelp.eu EMU-10 BUILD2LC_GRAPH_CORE_REAL_PASS_FAST_SAFE
==========================================================

CO JE TO
--------
Ostry graficky rez po LB/KZ/KV. Neni to dalsi jen loader build.

ZMENY V KODU
------------
1) XEX/OBX fake-fast nahravani zustava zapnute jen pro emulovany loader.
   CLOAD/CSAVE/kazeta zustava bez fake zrychleni.

2) ATR/D1 fake-fast zustava jen pro diskove SIO cteni.
   Hra po nahrani nesmi bezet naspeedovane.

3) VBXE SR/HR/LR graficky overlay:
   - graficke pixely berou primarne XDL OV paletu,
   - MAP atributova mapa zustava pro Atari PF barvy a prioritu,
   - MAP uz nema naslepo prehazovat graficky overlay do spatne/magenta palety.

4) Graph transparent guard:
   - u plneho grafickeho XDL recordu priority=$FF se barva 0 nesmi automaticky rozpadnout do pozadi,
   - cil: rozpadle/necely title/intra obrazky.

5) SR stripe filtr BUILD2LC:
   - obecny obsahovy filtr izolovane vodorovne SR artefakt-radky,
   - cil: Night Driver duch/pruh u auta,
   - bez screen-paintu a bez nazvu hry.

6) Donkey Arcade / Donkey Junior PMG faze zustava z KV chranena.

KODY JSOU STEJNE.

V LOGU MUSI BYT
---------------
AtariHelp.eu EMU-10 BUILD2LC_GRAPH_CORE_REAL_PASS_FAST_SAFE pripraven
XEX FAST LOAD BUILD2LC: locked fake-fast XEX/OBX nahravani zapnuto
ATR D1 FAST LOAD BUILD2LC: fake-fast ATR/D1 nahravani zapnuto jen pri diskovem SIO cteni
VBXE XDL RENDER BUILD2LC
VBXE HR/SR XDL RENDER BUILD2LC
VBXE SR STRIPE FILTER BUILD2LC   (jen kdyz filtr realne zasahne)

TEST PLAN
---------
1) Donkey Arcade
   Ocekavani: nahravani rychle, clovicek/zebrik OK, zvuk OK, rychlost OK.
   Kdyz se to rozbije, STOP a posli LOG.

2) Donkey Junior
   Ocekavani: nahravani rychle, opicka/zebrik OK, zvuk OK, rychlost OK.
   Kdyz se vrati rychla hra nebo posun sprite, STOP a posli LOG.

3) Popeye VBXE
   Ocekavani: nahravani rychlejsi nez pred KZ/KY, grafika lepsi/stejna/horsi.
   Hlavne sleduj, jestli mizi magenta/rozhozena paleta.

4) rrumble.xex
   Ocekavani: pred-hrou obraz lepsi/stejny/horsi, hra videt ano/ne.

5) Night Driver
   Ocekavani: auto ano, zvuk ano, pruh/duch mensi/stejny/pryc.

6) Decathlon
   Ocekavani: telo/nohy lepsi/stejne/horsi, napis nahore lepsi/stejne/horsi.

7) Postcard
   Ocekavani: okna barvy ano/ne, pruhy mensi/stejne/horsi.

JAK POSLAT VYSLEDEK
-------------------
Staci kratce:

BUILD2LC TEST
Donkey Arcade: ...
Donkey Junior: ...
Popeye VBXE: ...
rrumble: ...
Night Driver: ...
Decathlon: ...
Postcard: ...

Jeden spolecny LOG po cele sade.
Screenshot jen u Popeye/rrumble/Night Driver/Decathlon/Postcard, pokud tam bude rozdil.
