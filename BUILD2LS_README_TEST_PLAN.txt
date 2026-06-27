BUILD2LS_DECATHLON_NIGHTDRIVER_GRAPH_SAFE

Zaklad: BUILD2LR / BUILD2LO potvrzeny stav.

Rozhodnuti:
- Rychlost nahravani uz dal netlacim. LR/LQ prineslo razantni zrychleni, ale u Popeye VBXE uz byla kratka prodleva klavesnice. Dalsi agresivni turbo by bylo riziko.
- Proto BUILD2LS jde zpet na grafiku Decathlon + Night Driver.

Zmeny:
1) Decathlon / horni nastenka
- ANTIC CH6/7 s high CHBASE (napr. $12) zkousi jemnejsi HSCROL scale.
- Cil: nastenka nesmi zustat jen WELCOME, ma se rozhybat scroll text.
- Neni to podle nazvu hry, podminka je ANTIC CH6/7 + high CHBASE.

2) Decathlon / telo + nohy atleta
- PRIOR=$04 dostal opatrny P2/P3 line guard pres uzke PF0/PF1 linky.
- PRIOR=$01 cesta pro River Raid zustava beze zmen.
- Cil: telo/nohy atleta drzet vic pohromade.

3) Night Driver
- Silnejsi VBXE SR low-origin stripe filtr pro spodni pruh.
- Cil: odstranit sum/pruh dole bez ruseni auta a zvuku.

Co se nemeni:
- River Raid TOP cesta: chranena.
- VBXE loader rychlost z LR: zachovana.
- Popeye/W3D grafika: nehlasim jako opravena.
- UI / klavesnice / joystick / kazeta: beze zmen.
- CLOAD / CSAVE: beze zmen.
- KODY JSOU STEJNE.

V logu hledej:
AtariHelp.eu EMU-10 BUILD2LS_DECATHLON_NIGHTDRIVER_GRAPH_SAFE pripraven
ANTIC CH6 HSCROL SCALE BUILD2LS
GTIA PRIOR04 P2P3 LINE GUARD BUILD2LS
VBXE SR STRIPE FILTER BUILD2LC

V logu nesmi byt:
ANTIC HSCROL PHASE BUILD2LN
GTIA PMG EARLY HBLANK BUILD2LN

TEST:
1) River Raid
- musi zustat excelentni TOP
- nahravani musi zustat rychle

2) Decathlon
- nastenka: scroll text / jen WELCOME / horsi
- atlet telo/nohy: lepsi / stejny / horsi
- zacatek hry: OK / kouse se / horsi

3) Night Driver
- auto: OK / horsi
- zvuk: OK / horsi
- spodni pruh/sum: lepsi / stejny / horsi

4) Popeye VBXE a W3D jen rychla regrese
- nahravani zustalo rychle / horsi
- klavesnice po startu OK / kouse se vic
