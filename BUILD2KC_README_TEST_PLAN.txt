AtariHelp.eu EMU-10 BUILD2KC_NIGHTDRIVER_SR_LEADING_NOISE_GUARD_SAFE

Navazuje na BUILD2KB/JZ/JX.

STAV Z TESTU KB:
- Commando: Arnold ANO + hra ANO.
- Mission: navrat k puvodnimu, hra funguje, grafika vykreslena, ale mohla by byt lepsi.
- Night Driver: auto uz je, ale u auta zustava uzky horizontalni sum/caj.
- Decathlon: bez zmeny.

CO JE ZMENENO:
- Chranim Commando/JX cestu: XDL render without blitter zustava.
- Chranim Mission/KB cestu: weak/plain hry maji zamceny jen VBXE render overlay, IO se nelame.
- Pridan obecny VBXE SR leading-noise guard:
  pokud platny XDL SR record kresli dolni cast obrazovky z velmi nizke VBXE VRAM adresy hned za XDL tabulkou,
  preskoci se pouze prvni podezrela scanline tohoto SR overlaye.
- Cil: zmensit/odstranit uzky caj u auta v Night Driveru bez screen-paintu a bez hacku podle nazvu hry.

KODY JSOU STEJNE
BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.

CO SE NEMENI:
- UI, kazeta, joystick, loader.
- Commando Arnold obraz neni vlozeny obrazek.
- Decathlon zatim neni opravovan.

TEST PLAN:
1) Commando
   Ocekavam: Arnold ANO + hra ANO.
   Pokud Arnold zmizi, netestuj dal a posli LOG.

2) Night Driver
   Ocekavam: auto zustane ANO; uzky caj/sum u auta bude mensi nebo pryc.
   Napis: auto ano/ne, sum mensi/stejny/vetsi.
   Posli LOG.
   V LOGu hledat: VBXE SR LEADING NOISE GUARD BUILD2KC

3) Mission
   Ocekavam: hra po intru porad nabehne, grafika nesmi byt horsi nez KB.
   Napis: Mission stejna/lepsi/horsi.

4) Decathlon
   Ocekavam: bez regrese, pravdepodobne stale bez opravy.
   Napis: regrese ano/ne.
