AtariHelp.eu EMU-09 FIX186_REALOS_G7_TUNNEL_CLIP_GTIA_BARS_CORE

Navazuje na FIX185.

Cil:
- Stabilizovat REAL OS BASIC po potvrzeni GTIA 9/10/11 BARS.
- Neresit ted HSCROL naslepo, protoze aktualni fail je BASIC ERROR-141 pri rucnim GRAPHICS 7 programu.
- Pridat bezpecny G7 tunnel test a ochranu pasted BASIC programu proti zjevnym souradnicim mimo rozsah.

Zmeny:
- Build tag: FIX186_REALOS_G7_TUNNEL_CLIP_GTIA_BARS_CORE.
- Klavesnice zustava beze zmeny.
- GTIA 9/10/11 BARS zustavaji z FIX184/FIX185.
- Pridano tlacitko G7 TUNNEL 96 TEST.
- G7 TUNNEL 96 TEST pouziva GRAPHICS 23, tedy plny GRAPHICS 7 bez textoveho okna, rozsah 160x96.
- BASIC TXT / VLOZIT PROGRAM dostal FIX186 G7 paste clamp:
  - GRAPHICS 7 se prepne na GRAPHICS 23, kdyz program pouziva Y 80..96.
  - FOR X ... TO 160 se upravi na TO 159.
  - FOR Y ... TO 96 se upravi na TO 95.
  - MX=160-X se upravi na MX=159-X.
  - MY=96-Y se upravi na MY=95-Y.
  - PLOT/DRAWTO s literalem X >159 nebo Y >95 se orezou do legalniho rozsahu.
- Snapshot ma novy radek: G7 CLIP FIX186.

Dulezite:
- HSCROL $D404 neni v teto fazi hlavni pricina konkretniho failu. Log FIX185 ukazal ERROR-141 AT LINE 55 po BASIC paste. To je chyba rozsahu souradnic v BASICu, ne okamzite GTIA/HSCROL problem.
- Pokud se ma pozdeji resit realne horizontalni jemne scrollovani, musi se delat obecne v ANTIC rendereru pro XEX hry, ne jako hack pro tento BASIC test.

Test plan pro Reneho:
1. REAL OS BASIC AUTO BOOT.
2. G7 SPLIT 80 TEST - ma zustat OK.
3. G7 FULL 96 TEST - ma zustat OK.
4. G7 TUNNEL 96 TEST - nove tlacitko, ma kreslit cary dokola/tunel bez ERROR-141.
5. Zkus znovu vlozit svuj rucni G7 program pres VLOZIT PROGRAM.
6. GTIA 9 BARS / GTIA 10 BARS / GTIA 11 BARS jen kontrolne.
7. Snapshot/log: hledat G7 CLIP FIX186, GTIA BARS FIX186, ANTIC GRID.
