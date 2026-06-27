AtariHelp.eu EMU-10 BUILD2LP_VBXE_LOADER_TURBO_SAFE

ZAKLAD:
- BUILD2LO je potvrzeny rollback po LN: River Raid zpet TOP.
- Decathlon zustava otevreny: telo/nohy oddelene, nastenka nescrolluje.
- Night Driver zustava otevreny: pruh/sum dole.
- Popeye VBXE zustava otevreny graficky: prvni intro a hra nejsou finalne spravne.

PROC BUILD2LP:
Rene povolil jediny FAKE: rychlost nahravani. BUILD2LP proto nesaha na grafiku her, ale zrychluje pouze loaderove cesty:
1) XEX/OBX mnoho-zaznamovy VBXE loader:
   - Popeye VBXE ma stovky malych XEX segmentu/INITAD.
   - BUILD2LL ukoncil fake-fast uz pri cur=2/624, proto zbytek lezl pomalu.
   - BUILD2LP u mnoho-zaznamovych XEX drzi fake-fast az do EOF/poslednich dat.
   - CLOAD/CSAVE se nemeni.
2) ATR/D1 disk:
   - jemne silnejsi ATR/D1 fake-fast jen pri aktivnim D1: SIO prenosu.
   - cil: W3D ATR a podobne pomale VBXE diskove hry.
3) VBXE blitter log:
   - ztlumen detail dlouhych BCB chainu, protoze log-spam u VBXE her brzdi test.
   - samotny blitter a obrazova logika se tim nemeni.

NEMENI SE:
- UI / klavesnice / joystick / kazeta / skin.
- CLOAD / CSAVE / WAV/CAS.
- River Raid PRIOR=$01 z LL.
- XEX turbo exit pro male/bezne XEX z LL.
- LJ CHBASE 512.
- LM overlap/HSCROL LMS.
- Zadny screen-paint, zadny RAM inject, zadny hack podle nazvu hry.
- Testovaci XEX: KODY JSOU STEJNE. Pridany je jen Popeye VBXE XEX do test_assets jako test asset.

LOG MARKERY:
- AtariHelp.eu EMU-10 BUILD2LP_VBXE_LOADER_TURBO_SAFE pripraven
- XEX MANY-RECORD TURBO BUILD2LP
- VBXE BLITTER FAST BUILD2LP
- ATR D1 FAST LOAD BUILD2LC / BUILD2HW muze zustat, ale rychlost je LP jemne zvednuta

NESMI BYT V LOGU:
- ANTIC HSCROL PHASE BUILD2LN
- GTIA PMG EARLY HBLANK BUILD2LN

TEST PLAN:
1) River Raid.xex
   - musi zustat TOP jako BUILD2LO.
   - protivnici ANO, scroll/text/barvy TOP.

2) Popeye (VBXE, PAL Version)(2).xex
   - sleduj jen nahravani: rychlejsi / stejne / horsi.
   - grafiku zatim nehlas jako opravenou; jen poznamenej intro/hra: stejne / lepsi / horsi.
   - v logu hledej XEX MANY-RECORD TURBO BUILD2LP.

3) W3D ATR
   - sleduj nahravani: rychlejsi / stejne / horsi.
   - grafiku zatim nehlas jako opravenou.

4) Decathlon
   - ocekavani: stejne jako LO.
   - kdyz se zmeni, poslat screenshot + log.

5) Night Driver
   - ocekavani: stejne jako LO.
   - pruh dole porad otevreny.

FAIL RETURN:
- Pokud River Raid nebude TOP, vratit se na BUILD2LO.
- Pokud Popeye/W3D nahravani bude rychlejsi bez rozbiti River Raid, BUILD2LP je novy loader-safe bod.
