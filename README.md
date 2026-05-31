# AtariHelp.eu EMU-09 FIX60 PITSTOP CLEAN SPLIT + RIVER SAFE CORE

FIX60 navazuje na mobilní testy FIX59:

- Pitstop II: spodní půlka už seděla lépe, ale horní půlka a prostřední bílo-zelené šumy byly z přechodových řádků v road bufferech. FIX60 drží spodní půlku na $7000, horní na $7100 a filtruje transientní text/garbage řádky jen ve speciálním Pitstop split rendereru.
- River Raid: obraz běžel, ale letadlo bylo vizuálně mimo vodní koridor a kolize zabíjely hned po startu. FIX60 drží aircraft PMG bezpečněji uvnitř detekované vody a změkčuje River player/playfield kolize, když je střed letadla ve vodě.
- Obecný XEX core zůstává beze změny směrem k univerzálnímu emulátoru: DLIST resolver, DLI/CHBASE po řádcích, bitmapové ANTIC módy a PMG diagnostika.

Build tag: `FIX60_PITSTOP_CLEAN_SPLIT_RIVER_SAFE_CORE`.
