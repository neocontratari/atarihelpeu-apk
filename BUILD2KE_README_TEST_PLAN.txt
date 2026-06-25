AtariHelp.eu EMU-10 BUILD2KE_CONSOLIDATED_XDL_PMG_PIPELINE_PASS
================================================================

ZAKLAD / STATUS
---------------
Navazuje na BUILD2KB/JZ/JX, ne na KC/KD.
KC a KD nejsou brane jako smer. KD PMBASE=$00 zasah je pryc.

CHRANENE BODY
-------------
- Commando: Arnold ANO + hra ANO musi zustat.
- Mission: navrat z KB musi zustat, hra po intru nabehne a grafika nesmi byt horsi.
- Tetris VBXE a Heartlight/HL nesmi byt regresi.

CO JE OPRAVENE / ZMENENE
------------------------
1) Night Driver / VBXE SR low-origin sanitize
   Log ukazal XDL base=$00000 a SR record v dolni casti obrazu:
   y kolem 182, ovAddr velmi nizka ($00010), step 256.
   Predchozi KD preskocil jen 2 radky, coz nestacilo.
   BUILD2KE preskoci cele nizke zdrojove pasmo az pod $0800.
   Cili na uzky vodorovny caj u auta bez hacku podle nazvu hry a bez screen-paintu.

2) Decathlon / obecny PMG software override
   KD PMBASE=$00 DMA potlaceni zhorsilo obraz, proto je odstranene.
   BUILD2KE misto toho resi obecny pripad:
   kdyz P/M DMA latch pro radku prinese 0, ale software uz zapsal GRAFP/GRAFM,
   software multiplex smi nulovy DMA latch prebit.
   PMBASE, D640/D740 ani programovy tok se nelamou.

KODY JSOU STEJNE
----------------
BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.

UI / KAZETA / JOYSTICK / LOADER
-------------------------------
Beze zmen.

TEST PLAN - NE MIKROKOLECKO
---------------------------
1) Night Driver
   Ocekavani: auto zustane videt, uzky caj/sum u auta bude pryc nebo aspon jasne mensi.
   Pokud caj zustane stejny, poslat LOG / CHYBA a napsat: Night Driver caj stejny.

2) Decathlon
   Ocekavani: nesmi byt horsi nez KB/KD; pokud se PMG software override trefil,
   atlet/vrstvy budou lepsi nebo mene rozhazene.
   Pokud je stejne/horsi, poslat LOG / CHYBA a napsat: Decathlon stejny/horsi.

3) Rychla ochranna kontrola jen pokud mas chut/cas
   Commando: Arnold ANO + hra ANO.
   Mission: hra po intru nabehne, grafika neni horsi nez KB.

LOG MARKERY
-----------
AtariHelp.eu EMU-10 BUILD2KE_CONSOLIDATED_XDL_PMG_PIPELINE_PASS pripraven
VBXE SR LOW-ORIGIN SANITIZE BUILD2KE
GTIA PMG SOFTWARE OVERRIDE BUILD2KE

POKUD TO NEPOMUZE
-----------------
Nepokracovat dalsim mikrobuildem.
Dalsi spravny krok je hlubsi audit/vetsi oprava ANTIC/GTIA/display-list pipeline.
