AtariHelp.eu EMU-10 BUILD2LT_GRAPH_CORE_RASTER_STRIPE_SAFE

Zaklad: BUILD2LR. BUILD2LS je zahozena, protoze sice vycistila cast Decathlonu, ale ztratila text a neopravila atleta.

Co zustava:
- River Raid TOP + rychle nahravani z LR chraneno.
- Popeye/W3D rychlost loaderu z LR zustava.
- UI / klavesnice / joystick / kazeta beze zmen.
- CLOAD / CSAVE beze zmen.
- KODY JSOU STEJNE.

Nove zmeny:
1) GTIA EARLY HBLANK LINE START BUILD2LT
- ranne GTIA/PMG zapisy v prvni casti HBLANKu se mapuji od x=0, ne az od x=32.
- cil: Decathlon text/nastenka a PMG faze atleta bez herniho hacku.

2) VBXE SR LOW-ORIGIN SANITIZE BUILD2LT
- u nizke SR VRAM oblasti se preskoci delsi hlavickova/mirrored zona do $1400, max. 20 radku.
- cil: Night Driver pruh/sum pres auto a okraje silnice.

Popeye VBXE grafika:
- v tomto build se jeste nehlasi jako opravena; testovat jen regresi rychlosti a jestli se nezhoršila.

V LOGU HLEDEJ:
AtariHelp.eu EMU-10 BUILD2LT_GRAPH_CORE_RASTER_STRIPE_SAFE pripraven
GTIA EARLY HBLANK LINE START BUILD2LT
VBXE SR LOW-ORIGIN SANITIZE BUILD2LT

V LOGU NESMI BYT:
ANTIC HSCROL PHASE BUILD2LN
GTIA PMG EARLY HBLANK BUILD2LN
ANTIC CH6 HSCROL SCALE BUILD2LS
GTIA PRIOR04 P2P3 LINE GUARD BUILD2LS

TEST:
1) River Raid
- musi zustat excelentni TOP vcetne nahravani.

2) Night Driver
- pruh/sum pres auto: lepsi / stejny / horsi
- auto: OK / horsi
- zvuk: OK / horsi

3) Decathlon
- horni nastenka: text/modry scroll lepsi / stejny / horsi
- atlet telo+nohy: lepsi / stejny / horsi
- dolni scroll/Activision cast: objevila se / stejna / horsi

4) Popeye VBXE / W3D
- nahravani zustalo rychle / horsi
- klavesnice OK / kouse se vic
- grafika stejna / horsi

Kdyz River Raid nebo loader spadne, vracime BUILD2LR. Kdyz Night Driver pruh zmizi nebo se zmensi, LT je potvrzeny graficky smer.
