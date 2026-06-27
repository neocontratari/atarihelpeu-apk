AtariHelp.eu EMU-10 BUILD2LJ_CHBASE_512_ANTIC6_DECATHLON_TEXT_CORE

Zaklad: BUILD2LH.
BUILD2LI neberu jako uspesny smer - pozdni PMG latch se v teto oprave nepouziva.

Co je zmenene:
- Jen ANTIC znakovy renderer pro mode 6/7.
- Mode 6/7 ma jen 64 znaku, proto CHBASE muze byt na 512B hranici.
- Stary kod pouzival masku &FC pro vsechny znakove mody a tim posilal CHBASE=$12 na $1000.
- BUILD2LJ pouziva pro ANTIC 6/7 masku &FE, tedy CHBASE=$12 cte font z $1200.
- Decathlon podle XEX pise CHBASE=$12, proto je to prime podezreni pro rozbity horni scoreboard/nastenku.

Co se NEMENI:
- UI / klavesnice / joystick / kazeta / skin beze zmen.
- CLOAD / CSAVE beze zmen.
- PMBASE=$00 zustava povoleny jako v LH.
- LG zero guard zustava pryc.
- PRIOR tabulka zustava LH.
- XEX test assety: KODY JSOU STEJNE.

Log markery:
- AtariHelp.eu EMU-10 BUILD2LJ_CHBASE_512_ANTIC6_DECATHLON_TEXT_CORE pripraven
- ANTIC CHBASE 512 BUILD2LJ: mode 6/7 bere CHBASE=$12 jako $1200; stary 1K mask by cetl $1000
- GTIA PMBASE ZERO DMA ALLOWED BUILD2LH
- GTIA PRIOR TABLE BUILD2LH

Test plan:
1) Nahraj The Activision Decathlon.xex.
2) Hlavni kontrola: horni scoreboard/nastenka/text.
   - napis na tabuli: lepsi / stejny / horsi
   - cerna tabule/ramecek: lepsi / stejny / horsi
   - atlet dole: lepsi / stejny / horsi
3) Rychla regrese:
   - Donkey Arcade / Junior: postavy a zebriky OK / rozbite
   - Commando: Arnold + hra OK / rozbite
   - Mission: vraci se do hry OK / rozbite
4) Posli screenshot Decathlon horni casti a cely LOG / CHYBA.

Poznamka:
Tohle neni screen-paint ani hack podle nazvu hry. Je to obecna oprava adresace CHBASE pro ANTIC 6/7.
