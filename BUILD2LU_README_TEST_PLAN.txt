BUILD2LU_NIGHTDRIVER_CAR_PRESERVE_DECATHLON_AUDIT_SAFE

ZAKLAD
- Vychazi z BUILD2LT, ale BUILD2LS zustava zahozena.
- River Raid TOP a rychly loader z BUILD2LR/LQ jsou chranene.
- Loader, UI, klavesnice, joystick, kazeta, CLOAD/CSAVE: beze zmen.
- Testovaci XEX/ATR soubory: KODY JSOU STEJNE.

PROC BUILD2LU
- BUILD2LT dokazal u Night Driver odstranit vodorovny sum/pruh.
- Test ale ukazal, ze LT moc agresivne preskocil low-origin SR radky a ukrojil predek auta.
- LU proto zmensuje pevny SR skip z max. 20 radku na max. 15 radku a zbytek nechava na obsahovy stripe filtr.
- Decathlon zatim neni prohlasen za opraveny; pridan je audit CH6/HSCROL pro dalsi cilene reseni textove nastenky.

LOG MARKERY
Hledej:
- AtariHelp.eu EMU-10 BUILD2LU_NIGHTDRIVER_CAR_PRESERVE_DECATHLON_AUDIT_SAFE pripraven
- VBXE SR LOW-ORIGIN CAR PRESERVE BUILD2LU
- ANTIC CH6 DECATHLON AUDIT BUILD2LU

V logu nesmi byt:
- ANTIC HSCROL PHASE BUILD2LN
- GTIA PMG EARLY HBLANK BUILD2LN
- ANTIC CH6 HSCROL SCALE BUILD2LS
- GTIA PRIOR04 P2P3 LINE GUARD BUILD2LS

TEST PLAN
1) River Raid
- musi zustat excelentni TOP vcetne nahravani

2) Night Driver
- pruh/sum: pryc / vratil se / horsi
- predek auta: vratil se / stale ukrojeny / horsi
- zvuk: OK / horsi

3) Decathlon
- horni nastenka: jen WELCOME / lepsi scroll / horsi
- atlet telo+nohy: stejny / lepsi / horsi
- dolni Activision scroll: stejny / objevil se / horsi

4) Popeye VBXE / W3D
- nahravani zustalo rychle / horsi
- grafika stejna / horsi

NAVRAT
- Pokud River Raid nebo loader regreduje, vratit BUILD2LR.
- Pokud Night Driver pruh zustane pryc a auto se zlepsi, LU je novy Night Driver zaklad.
