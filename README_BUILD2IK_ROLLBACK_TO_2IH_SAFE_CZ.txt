AtariHelp.eu EMU-10 BUILD2IK_ROLLBACK_TO_2IH_SAFE

Toto neni nova oprava VBXE. Je to zachranny rollback balicek na posledni bezpecny zaklad BUILD2IH.

NEPOUZIVAT / ZAHODIT:
- BUILD2II_VBXE_RECORD_VC_TRANSPARENCY_SAFE
- BUILD2IJ_VBXE_XDL_STABLE_SPARSE_UNDERLAY_SAFE

PROC:
BUILD2II podle testu neprinesl viditelnou zmenu: HL sedy obdelnik zustal, Popeye zustal nekompletni, Commando/Arnold zustal problemovy.
BUILD2IJ byl pokus po slepe vetvi a neni oznacen jako stabilni.

CO ZUSTAVA Z DOBREHO ZAKLADU BUILD2IH:
- UI VBXE 130XE skin zachovan
- hitboxy klavesnice zachovany
- joystick zachovan
- NET HRY funkcni
- rychle ATR/D1 a rychly XEX loader zachovan
- TXT / TurboBasic rychle vkladani zachovano
- RAM320 + VBXE VRAM cesta zachovana
- BCB21 zaklad zachovan
- W3D/HL/Popeye/Commando testovat od tohoto bodu znovu cilene, ne pres BUILD2II/IJ

KODY JSOU ZMENENE pouze tim, ze index.html je navracen na BUILD2IH.
BASIC / Altirra testovaci kody JSOU STEJNE.

Dalsi pokracovani:
1) Nelepit dalsi probe/underlay pokusy naslepo.
2) Nejdriv analyzovat BCB chain / xdl record / layer priority z logu.
3) Cilit pouze jednu chybu na build: bud HL sedy obdelnik, nebo Popeye chybejici vrstva, nebo Commando Arnold intro.
4) Pokud neni jasne proc ma zmena zlepsit konkretni symptom, neposilat testerovi dalsi build.
