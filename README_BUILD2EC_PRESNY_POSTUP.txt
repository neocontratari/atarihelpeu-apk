ATARIHELP.EU EMU-10 BUILD2EC - PRESNY POSTUP PRO RENEHO
=========================================================

UCEL:
Oprava rozbiteho CLOAD po Decathlon/DV vetvi.
Nesahat zbytecne na CSAVE, XEX, Java appku, ikony ani hlavni menu.

KODY JSOU STEJNE
BASIC/Altirra srovnavaci kody se nemeni.

CO TENTO BALICEK MENI:
Pouze:
  app\src\main\assets\emu\index.html

CO TENTO BALICEK NEMENI:
  MainActivity.java
  AndroidManifest.xml
  ikony
  hlavni assets/index.html
  XEX picker
  CSAVE uloziste
  ROM soubory

PRESNY POSTUP:

1) Otevri GitHub Desktop.
2) Vyber repozitar/projekt AtariHelp EMU-10.
3) Nahoře dej:
   Repository -> Show in Explorer
4) Otevre se presna slozka projektu.
   V ni musis videt slozky/soubory:
   app
   gradle
   build.gradle / build.gradle.kts
   settings.gradle / settings.gradle.kts
5) Do teto otevrene slozky zkopiruj VSE z tohoto ZIPu:
   SPUSTIT_BUILD2EC_CLOAD_OPRAVA.cmd
   _EMU10_BUILD2EC_PAYLOAD
   README_BUILD2EC_PRESNY_POSTUP.txt
6) V te stejne slozce dvakrat klikni na:
   SPUSTIT_BUILD2EC_CLOAD_OPRAVA.cmd
7) CMD musi napsat HOTOVO.
8) Otevri GitHub Desktop.
9) V seznamu zmen ma byt hlavne:
   app/src/main/assets/emu/index.html
10) Do Summary napis:
    BUILD2EC CLOAD core restore
11) Dej:
    Commit to main
12) Dej:
    Push origin
13) Pak na webu/GitHub Actions udelas/vezmes APK jako vzdy.

KDYZ CMD NAPISE CHYBU:
- Nejsi v koreni projektu.
- Znovu otevri GitHub Desktop -> Repository -> Show in Explorer.
- Obsah ZIPu zkopiruj do slozky, ktera se otevře.

TESTOVANI:
Netestovat kratky demo kod. Testovat tvoje realne kody, CSAVE, CLOAD, XEX hry.
Poradi:
1) XEX hra, ktera predtim jela.
2) CSAVE tvojeho realneho programu.
3) CLOAD z ulozene kazety.
4) LIST a RUN.

KDYZ SELZE:
Posli LOG a napis presne:
ERROR 138 / ERROR 143 / ERROR 21 / VISI BEZ CHYBY / LIST SPATNY / XEX REGRESE
