AtariHelp.eu EMU-10 BUILD2LN_PMG_EARLY_HBLANK_AND_HSCROL_PHASE_DECATHLON_SAFE

ZAKLAD:
- navazuje na BUILD2LM/LL
- LM/LL je potvrzeny dobry bod pro River Raid: River Raid TOP, nahravani/chytani lepsi
- Decathlon v LM zustal beze zmen
- Night Driver pruh zustal beze zmen

CO JE NOVE:
1) GTIA PMG EARLY HBLANK BUILD2LN
   - ranne PMG HPOS/SIZE/GRACTL segmenty do x48 se berou jako priprava cele scanline
   - pozdejsi viditelne segmenty zustavaji paprskove
   - cil: Decathlon telo/nohy bez LK globalniho freeze

2) ANTIC HSCROL PHASE BUILD2LN
   - HSCROL kopiruje z x+hscroll misto x-hscroll
   - cil: Decathlon horni nastenka, aby se rozhybal scroll text

CO SE NEMENI:
- River Raid PRIOR=$01 z LL zustava
- XEX turbo exit z LL zustava
- CHBASE 512 z LJ zustava
- LM player overlap zustava
- UI / klavesnice / joystick / kazeta beze zmen
- CLOAD / CSAVE beze zmen
- ATR disk turbo beze zmen
- Testovaci XEX: KODY JSOU STEJNE

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2LN_PMG_EARLY_HBLANK_AND_HSCROL_PHASE_DECATHLON_SAFE pripraven
GTIA PMG EARLY HBLANK BUILD2LN
ANTIC HSCROL PHASE BUILD2LN
GTIA PRIOR01 BUILD2LL
XEX FAST LOAD BUILD2LL
ANTIC CHBASE 512 BUILD2LJ

TEST PLAN:
1) River Raid
- musi zustat TOP jako LM/LL
- protivnici videt ANO
- grafika/barvy/scroll napisy nesmi byt horsi
- nahravani/chytani nesmi byt horsi

2) Decathlon
- telo/nohy atleta: lepsi / stejne / horsi
- horni nastenka: scrolluje text / jen WELCOME / horsi
- start hry: lepsi / stejny / horsi

3) Night Driver
- pruh dole: stejny / lepsi / horsi
- auto + zvuk: OK / horsi

KDYZ SE RIVER RAID ROZBIJE:
- okamzite rollback na BUILD2LM/LL
