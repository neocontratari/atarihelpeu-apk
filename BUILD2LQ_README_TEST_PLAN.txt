AtariHelp.eu EMU-10 BUILD2LQ_VBXE_AGGRESSIVE_LOADER_TURBO_SAFE

ZAKLAD:
- BUILD2LP/LO.
- River Raid TOP stav z LO/LP chranen.
- LN zustava mrtva, nevracime HSCROL PHASE ani PMG EARLY HBLANK.
- Decathlon grafiku tento build neopravuje.
- Night Driver pruh tento build neopravuje.
- Popeye VBXE grafiku tento build neopravuje, jen rychlost nahravani.

HLAVNI CIL:
- Jediny povoleny fake podle Reného: rychlost nahravani.
- Normalni hry uz nacitaji rychle; problem jsou VBXE tituly typu Popeye VBXE XEX a W3D ATR.

ZMENA BUILD2LQ:
1) Popeye / VBXE mnoho-zaznamovy XEX
- BUILD2LP stale vypnul turbo fail-safe po cca 2400 framech, kdyz XEX zustal na cur=2/624 a PC bylo v INITAD kodu.
- BUILD2LQ pro many-record XEX nevypina turbo pred EOF timto fail-safe.
- Turbo krok pro many-record XEX zvysen na 1800 frame/RAF.
- CLOAD/CSAVE zustava mimo tuto vetev.

2) W3D / VBXE ATR
- ATR D1 turbo okno prodlouzeno.
- Diskovy turbo krok zvysen na 900 frame/RAF.
- Pro potvrzene VBXE ATR pridano kratke OS/D1 boot turbo, kdyz CPU bezi v OS/SIO oblasti $E000+.
- Neni to RAM inject ani patch hry; je to pouze zrychleni emulovaneho boot/read cekani.

3) Log performance
- PM audit summary se pri turbo fazi tlumi, aby obrovske logy nezpomalovaly VBXE loader.
- VBXE blitter log tlumeni z LP zustava.

NEMENI SE:
- UI, klavesnice, joystick, kazeta, CLOAD/CSAVE.
- River Raid PRIOR01 cesta zustava.
- Decathlon CHBASE/PMG cesta zustava.
- Test XEX kody jsou stejne.

V LOGU HLEDEJ:
AtariHelp.eu EMU-10 BUILD2LQ_VBXE_AGGRESSIVE_LOADER_TURBO_SAFE pripraven
XEX MANY-RECORD TURBO BUILD2LP
VBXE ATR BOOT TURBO BUILD2LQ
ATR D1 FAST LOAD BUILD2HW

V LOGU NESMI BYT:
ANTIC HSCROL PHASE BUILD2LN
GTIA PMG EARLY HBLANK BUILD2LN

TEST PLAN:
1) River Raid
- musi zustat TOP jako v LO/LP.
- pokud se zhorsi, BUILD2LQ zahodit a vratit LP/LO.

2) Popeye VBXE XEX
- porovnat rychlost nahravani proti LP.
- cekany vysledek: citelne rychlejsi.
- grafiku neocekavam opravenou.

3) W3D ATR
- porovnat rychlost nahravani proti LP.
- cekany vysledek: rychlejsi boot/load.
- grafiku neocekavam opravenou.

4) Decathlon / Night Driver
- jen kontrola, ze nejsou horsi.

KDYZ POSLAT LOG:
- kdyz Popeye/W3D porad trvaji dlouho, poslat LOG po 30-60 s cekani.
- hlavne potrebuju videt, jestli bezi VBXE ATR BOOT TURBO BUILD2LQ a jestli XEX u Popeye zustal v turbo az za cur=2/624.
