AtariHelp.eu EMU-10 BUILD2LM_PLAYER_OVERLAP_AND_HSCROL_DECATHLON_SAFE

ZAKLAD
- Navazuje na BUILD2LL, protoze uzivatel potvrdil:
  - River Raid je TOP, protivnici jsou videt, grafika ve hre sedi.
  - nahravani/chytani her je lepsi.
- LL opravy se zachovavaji:
  - PRIOR=$01 pro River Raid P2/P3.
  - rychle vypnuti XEX turbo po prechodu do hry.
  - LJ CHBASE 512 pro Decathlon horni text.

CO JE NOVE V BUILD2LM
1) GTIA player overlap normal bez PRIOR bit5
- Predchozi kod pri prekryvu P0/P1 nebo P2/P3 bez PRIOR bit5 vracel cernou barvu.
- To muze rozdelit jednobarevny sprite na kusy, kdyz hra sklada postavu ze dvou playeru.
- BUILD2LM v tomhle pripade neudela cernou diru a necha videt aktualni player.
- Cil: Decathlon telo/nohy atleta.
- Neni to hack podle nazvu hry a neni to screen paint.

2) ANTIC HSCROL LMS krok
- HSCROL fetch zustava sirsi kvuli jemnemu posuvu.
- Po dokonceni znakoveho radku se ale memory scan counter posune jen o viditelne bytes, ne o extra fetch bytes.
- Cil: Decathlon horni nastenka / scrolling text.

CO SE NEMENI
- UI, skin, klavesnice, joystick, kazeta: beze zmen.
- CLOAD/CSAVE: beze zmen.
- ATR disk turbo: beze zmen.
- Testovaci XEX soubory: KODY JSOU STEJNE.
- Night Driver pruh zatim nelakuji jako opraveny.

LOG MARKERY
Hledej v LOG / CHYBA:
- AtariHelp.eu EMU-10 BUILD2LM_PLAYER_OVERLAP_AND_HSCROL_DECATHLON_SAFE pripraven
- GTIA PLAYER OVERLAP NORMAL BUILD2LM
- ANTIC HSCROL LMS BUILD2LM
- GTIA PRIOR01 BUILD2LL
- XEX FAST LOAD BUILD2LL
- ANTIC CHBASE 512 BUILD2LJ

TEST PLAN
1) River Raid
- protivnici: ANO / NE
- grafika ve hre: OK / horsi
- nahravani/chytani: OK / horsi

2) Decathlon
- nacteni/zacatek: OK / kouse se / horsi
- horni nastenka: zacina scrollovat text / jen WELCOME / horsi
- atlet telo+nohy: drzi vic pohromade / stejne oddelene / horsi
- barvy a stadion: OK / horsi

3) Night Driver
- auto: ANO / NE
- spodni sum/cara: stejna / lepsi / horsi
- zvuk: OK / horsi

4) Regrese
- Donkey Junior
- Commando
- Mission

KDYZ SE NECO ROZBIJE
- Pokud River Raid zhorseny: vratit se na BUILD2LL jako potvrzeny dobry bod.
- Pokud Decathlon stejny: dalsi rez nema jit zpet na PMBASE/PRIOR, ale hledat DLI/HSCROL/PMG vertikalni fazi pres konkretni log.
