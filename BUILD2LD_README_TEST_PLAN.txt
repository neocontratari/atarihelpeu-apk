AtariHelp.eu EMU-10 BUILD2LD_ANTIC_DMA_HALT_DLI_VBL_TIMING_CORE

CO JE ZMENENE:
- Navazuje na BUILD2LC graficky core a KZ/KV/KY/KZ rychle nahravani.
- Pridana ANTIC DMA/HALT kradez CPU cyklu v jednovlaknove scanline smycce.
- Cilem je realny problem, ktery popsal tester: Decathlon a podobne non-VBXE/DLI hry nemaji bez HALT cyklu spravne casovani barev/spritu.
- DLI/VBI zustava ve stejne kooperativni smycce; PAL frame drzi 312 radku, VBI line 248.
- Pro ANTIC mode 4/5 se krade 7 CPU cyklu na radku jako prvni presny Decathlon krok.
- Ostatni graficke mody maji konzervativni HALT budget podle mode/DMACTL/PMG, aby se nerozbily Donkey Arcade/Junior.
- Fast XEX/OBX a ATR/D1 zustava jen pro nahravani.
- CLOAD/CSAVE/kazeta zustava bez fake zrychleni.
- Donkey Arcade / Donkey Junior PMG faze zustava chranena.
- Bez screen-paintu. Bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

V LOGu MUSI BYT:
AtariHelp.eu EMU-10 BUILD2LD_ANTIC_DMA_HALT_DLI_VBL_TIMING_CORE pripraven
ANTIC DMA HALT BUILD2LD
VBL/DLI TIMING BUILD2LD

TEST:
1) Donkey Arcade: clovicek/zebrik OK, zvuk OK, rychlost OK, nahravani rychle.
2) Donkey Junior: opicka/zebrik OK, zvuk OK, rychlost OK, nahravani rychle.
3) Decathlon: telo/nohy lepsi/stejne/horsi, napis nahore lepsi/stejny/horsi, rychlost OK/RYCHLA.
4) Night Driver: auto ano/ne, pruh mensi/stejny/pryc, zvuk ano/ne.
5) Postcard: okna barvy ano/ne, pruhy mensi/stejne/horsi.
6) Popeye VBXE: grafika lepsi/stejna/horsi, nahravani rychlejsi/stejne.
7) rrumble: obraz pred hrou lepsi/stejny/horsi, hra videt ano/ne.

POKUD SE ROZBIJE DONKEY ARCADE/JUNIOR:
- Poslat LOG hned po testu.
- Vracet LD HALT budget zpet nebo ho omezit jen na ANTIC 4/5.

POKUD SE DECATHLON POHNE:
- Poslat screenshot Decathlon + LOG.
- Dalsi krok ma byt doladeni rozlozeni HALT cyklu v radce, ne dalsi paletovy hack.
