AtariHelp.eu EMU-10 BUILD2LO_RIVERRAID_TOP_ROLLBACK_LN_SAFE

STATUS:
- LN je zahozena, protoze test ukazal: River Raid horsi scrollovani/text a grafika uz nebyla TOP.
- BUILD2LO vychazi z potvrzeneho BUILD2LM/LL, ne z LN.
- Cilem tohoto overlaye je vratit a zamknout potvrzeny TOP stav River Raid + lepsi nahravani.
- Decathlon zatim NEHLASIM jako opraveny: telo/nohy a scrolling nastenka zustavaji otevrene.
- Night Driver NEHLASIM jako opraveny: spodni pruh/sum caru zustava otevreny.

ZACHOVANO Z FUNKCNI VETVE:
- LL PRIOR=$01 oprava: River Raid protivnici videt.
- LL XEX turbo exit: hra se po nacteni chyta rychleji, turbo nevleze do hry.
- LJ CHBASE 512: Decathlon horni text se zlepsil proti puvodnimu stavu.
- LM player overlap + HSCROL LMS: ponechano, protoze River Raid byl po LM potvrzen TOP.

ZAHOZENO Z LN:
- ANTIC HSCROL PHASE BUILD2LN: zruseno, protoze rozbilo River Raid scroll/text.
- GTIA PMG EARLY HBLANK BUILD2LN: zruseno, protoze je soucast stejne neuspesne LN vetev a Decathlon nezlepsilo.

NEMENI SE:
- UI / klavesnice / joystick / kazeta / skin.
- CLOAD / CSAVE.
- ATR disk turbo.
- Testovaci XEX: KODY JSOU STEJNE.
- Bez screen-paintu, bez RAM injectu, bez hacku podle nazvu hry.

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2LO_RIVERRAID_TOP_ROLLBACK_LN_SAFE pripraven
GTIA PRIOR01 BUILD2LL
XEX FAST LOAD BUILD2LL
ANTIC CHBASE 512 BUILD2LJ
GTIA PLAYER OVERLAP NORMAL BUILD2LM
ANTIC HSCROL LMS BUILD2LM

NESMI BYT V LOGU:
ANTIC HSCROL PHASE BUILD2LN
GTIA PMG EARLY HBLANK BUILD2LN

TEST PLAN:
1) River Raid:
   - musi byt zpet TOP jako v LM
   - protivnici musi byt videt
   - scroll/text a barvy musi byt OK
   - nahravani/chytani nesmi byt horsi

2) Decathlon:
   - ocekavani: pravdepodobne jako LM, ne final fix
   - zapsat: telo/nohy stejne/lepsi/horsi
   - zapsat: nastenka WELCOME/scroll/horsi

3) Night Driver:
   - ocekavani: pruh dole pravdepodobne stejny
   - auto + zvuk nesmi byt horsi

KDYZ RIVER RAID NENI TOP:
- poslat screenshot + LOG/CHYBA.
- BUILD2LO by mel byt prakticky LM baseline s rollbackem LN; pokud neni, testuje se spatny overlay nebo je nova regrese.
