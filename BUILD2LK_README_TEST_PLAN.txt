AtariHelp.eu EMU-10 BUILD2LK_STABLE_PMG_HPOS_SIZE_PLUS_LJ_TEXT_CORE
==================================================================

ZAKLAD
- Navazuje na BUILD2LJ, protoze LJ viditelne zlepsil Decathlon horni text / WELCOME.
- BUILD2LI pozdni PMG latch se NEPOUZIVA jako uspesny smer.
- BUILD2LG PMBASE zero guard zustava zahozene.

CO JE ZMENENO
- Zachovana LJ oprava ANTIC 6/7 CHBASE 512B: CHBASE=$12 -> $1200.
- Novy rez je jen v P/M rendereru:
  Pri zapnutem P/M DMA se HPOS/SIZE registr pro danou scanline bere ze snapshotu,
  ne z pozdnich viditelnych segmentu uprostred radky.
- GRAFP/GRAFM segmenty zustavaji aktivni.
- Cil: Decathlon atlet - telo/nohy se nesmi rozpadat na ruzna mista.

CO NENI ZMENENO
- UI / skin / klavesnice / joystick / kazeta: beze zmen.
- CLOAD / CSAVE: beze zmen.
- Testovaci XEX: KODY JSOU STEJNE.
- Zadny screen-paint, zadny hack podle nazvu hry.

LOG MARKERY
Hledej v LOG / CHYBA:
- AtariHelp.eu EMU-10 BUILD2LK_STABLE_PMG_HPOS_SIZE_PLUS_LJ_TEXT_CORE pripraven
- GTIA PMG STABLE POS BUILD2LK
- ANTIC CHBASE 512 BUILD2LJ
- GTIA PMBASE ZERO DMA ALLOWED BUILD2LH

TEST PLAN PRO RENÉHO
1) Nainstaluj APK z GitHub Actions / overlaye BUILD2LK.
2) Spust The Activision Decathlon.xex.
3) Porovnej hlavne proti BUILD2LJ:
   - horni text / WELCOME / nastenka: lepsi / stejny / horsi
   - atlet telo + nohy: lepsi / stejny / horsi
   - atlet pri pohybu doprava: drzi pohromade / porad se trha
4) Regresni minimum:
   - Donkey Junior: postavy u zebriků OK / rozbite
   - Commando Arnold + hra: OK / rozbite
   - Mission: vraci se do hry OK / rozbite
   - Night Driver: auto ano/ne, pruh stejny/horsi/lepsi
5) Pokud je Decathlon horsi, vratit se na BUILD2LJ, ne na LI/LG.

CO POSLAT ZPATKY
- Screenshot Decathlon presne jako u LJ.
- Cely LOG / CHYBA txt.
- Jedna veta: BUILD2LK telo/nohy lepsi / stejne / horsi.
