AtariHelp.eu EMU-10 / VBXE 130XE
BUILD2JU_VBXE_TILE_UNDERLAY_PM_AUDIT_SAFE

NAVAZUJE NA:
- BUILD2JT_PM_VISIBILITY_AUDIT_SAFE
- BUILD2JS_GTIA_PM_DMA_LATCH_SAFE jako posledni potvrzeny zaklad

KODY JSOU STEJNE
- BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.
- Kazeta/CLOAD/CSAVE/UI/joystick/loader nejsou cilene menene.

CO JE ZMENENE:
1) Pridana obecna VBXE tile-surface cesta pro pred-XDL obraz.
   Kdyz hra realne vykresli mnoho malych BCB blitu do VBXE VRAM se stride 512,
   emulator si z toho udela kandidata obrazove plochy.
2) Pokud jeste neni platny XDL a Atari/ANTIC obraz uz je aktivni, tato plocha se kresli jen jako PODKLAD
   do pixelu pozadi COLBK. ANTIC text/sprity zustavaji nahore.
3) Duvod: Commando intro ma podle logu tisice realnych BCB 8x8 blitu do VBXE VRAM, ale v EMU je videt
   jen ANTIC text PRESS ANY KEY TO START. BUILD2JU ma zkusit zobrazit Arnold obraz pod textem bez hacku podle nazvu hry.
4) PM audit z BUILD2JT zustava aktivni pro Night Driver / Decathlon / Mission.

CO NENI ZMENENE:
- Zadny screen paint podle jmena hry.
- Zadny Arnold obraz vlozeny do kodu.
- Zadny fake XEX/READY/CLOAD.
- Zadna herni vyjimka typu if Commando.
- Zadne vraceni mrtvych vetvi JO DMA / BCB19 / MSEL-OVSHIFT.

TEST 1: COMMando intro
1. Nainstaluj overlay pres GitHub Desktop.
2. Spust aplikaci.
3. V logu musi byt:
   AtariHelp.eu EMU-10 BUILD2JU_VBXE_TILE_UNDERLAY_PM_AUDIT_SAFE pripraven
4. Nahraj commando230810.xex.
5. Pockej na PRESS ANY KEY TO START.

SPRAVNY VYSLEDEK:
- V uvodu se ma objevit Arnold/Commando obraz jako v Altirre, text zustane nahore citelny.
- Hra po stisku klavesy/FIRE porad jede.

LOG MARKERY:
- VBXE TILE SURFACE BUILD2JU
- VBXE TILE UNDERLAY BUILD2JU
- VBXE BLITTER FAST DONE

KDYZ TO SELZE:
Posli screenshot uvodu a LOG / CHYBA.
Napis hlavne:
- je Arnold videt / neni videt / je caj
- jestli text PRESS ANY KEY TO START zustal citelny
- jestli hra po startu porad jede

TEST 2: Night Driver
1. Nahraj nightdriver_vbxe.xex.
2. Dojeď do hry a nech 20-40 sekund bez auta.

SPRAVNY VYSLEDEK IDEAL:
- Auto se objevi.

KDYZ AUTO NENI:
- Posli LOG. Dulezite markery:
  BUILD2JT PM SUMMARY nightdriver_vbxe.xex
  BUILD2JT PM DMA FIRST PLAYER
  BUILD2JT PM DMA FIRST MISSILE
  BUILD2JT PM REG

TEST 3: Decathlon
1. Nahraj The Activision Decathlon.xex.
2. Nech bezet obraz s atletem.

SPRAVNY VYSLEDEK IDEAL:
- Atlet/sprity budou blize Altirre.

KDYZ ZADNA ZMENA:
- Posli LOG s BUILD2JT PM SUMMARY pro Decathlon.

TEST 4: Mission
1. Nahraj mission.xex.
2. Porovnej, jestli hra jede a jestli barvy nejsou horsi nez v BUILD2JS/JT.

SPRAVNY VYSLEDEK:
- Hra jede, zadna nova regrese.

CO POSLAT ZPET:
- 4 kratke vety:
  Commando - Arnold ano/ne, hra jede ano/ne
  Night Driver - auto ano/ne
  Decathlon - zmena ano/ne
  Mission - jede ano/ne, barvy lepsi/horsi/nevim
- jeden LOG idealne po vsech testech, nebo samostatne LOGy pro hry kde je chyba.
