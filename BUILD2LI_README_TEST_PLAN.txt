AtariHelp.eu EMU-10 BUILD2LI_PMG_LATE_LATCH_DECATHLON_SCANLINE_CORE
====================================================================

ZAKLAD
- Navazuje primo na BUILD2LH_PMBASE_ZERO_GUARD_ROLLBACK_REAL_PRIORITY_CORE.
- BUILD2LG PMBASE zero guard zustava zruseny: PMBASE=$00 je povoleny, P/M DMA data se znovu ctou z realne oblasti $0400-$0700.
- UI, skin, klavesnice, joystick, XC12 kazeta, loader tlacitka: BEZE ZMEN.
- CLOAD/CSAVE: BEZE ZMEN, bez fake/turba.
- XEX/OBX/ATR fast-load zustava jen loader, ne hra.
- KODY JSOU STEJNE: testovaci XEX assety se nemenily.

CO SE MENILO
- Jen PMG/scanline jadro.
- V LH se P/M DMA latch delal uplne na zacatku scanline jeste pred rannym CPU/DLI oknem.
- To mohlo znamenat, ze DLI/WSYNC raster priprava nastavila PMBASE/GRACTL/HPOS/PRIOR uz pro aktualni playfield render, ale P/M data zustala o radku pozadu.
- BUILD2LI presouva pmDmaFetch az za ranne CPU/DLI/WSYNC okno a za snapTake(), tesne pred render radky.
- Cil neni screen-paint ani hack podle Decathlonu. Je to obecna scanline synchronizace PMG latch vs playfield snapshot.

PROC TENTO SMER
- Posledni test po LH: Decathlon ma atlety zpet, ale horni atlet se srovnava podle Y pozice a horni scoreboard/nastenka je spatne.
- To sedi spis na PMG/playfield scanline fazi nez na dalsi slepou paletu.
- Tento build nema prohlasovat Decathlon za hotovy bez testu.

CO MUSI BYT V LOGU
- AtariHelp.eu EMU-10 BUILD2LI_PMG_LATE_LATCH_DECATHLON_SCANLINE_CORE pripraven
- GTIA PMG LATE LATCH BUILD2LI
- BUILD2LI PM LATE-LATCH LINE
- Puvodni markery LH zustanou taky:
  - GTIA PMBASE ZERO DMA ALLOWED BUILD2LH
  - GTIA PRIOR TABLE BUILD2LH
  - BUILD2JT PM SUMMARY The Activision Decathlon.xex

TEST PLAN PRO RENEHO
1) Nainstaluj overlay pres GitHub Desktop stejne jako driv.
2) Spust appku, zapni LOG / CHYBA.
3) Nahraj The Activision Decathlon.xex.
4) Otestuj hlavne:
   - obraz na uvodni/horní casti,
   - horni atlet,
   - scoreboard/nastenka/napis,
   - telo/nohy pri vlastnim zavode,
   - jestli se srovnani meni podle Y pozice.
5) Kratka regrese:
   - Donkey Arcade / Donkey Junior: postavy u zebriků se nesmi rozbit.
   - Commando: Arnold + hra se nesmi rozbit.
   - Mission: musi se vratit do hry.
   - Night Driver: auto/zvuk/pruh poznamka, nehlasit jako hlavni cil.

JAK POSLAT VYSLEDEK
Posli kratce:

BUILD2LI TEST
Decathlon horni atlet: lepsi / stejny / horsi
Decathlon scoreboard/nastenka: lepsi / stejny / horsi
Decathlon telo/nohy pri zavode: lepsi / stejny / horsi
Donkey Arcade/Junior: OK / rozbite
Commando: OK / rozbite
Mission: OK / rozbite
Night Driver: auto ..., pruh ..., zvuk ...
LOG: prilozit cely log z LOG / CHYBA
Screenshot: hlavne Decathlon horni cast + zavod

KDYZ JE TO HORSI
- Vratit se na BUILD2LH jako posledni jisty stav.
- Nevracet BUILD2LG.
