ATARIHELP.EU EMU-10 BUILD2ED_REAL_WAV_IRG_AUDIT
================================================

CO TO JE
--------
CMD installer pro Reneho postup:
1) zkopirovat do korene projektu
2) spustit CMD
3) GitHub Desktop Commit/Push
4) WWW/GitHub Actions vyrobi APK

KAM TO ZKOPIROVAT
-----------------
Do korene projektu:
C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

Ve stejne slozce musi byt:
app
gradle
build.gradle nebo build.gradle.kts
settings.gradle nebo settings.gradle.kts

CO ZKOPIROVAT
-------------
Z tohoto ZIPu do korene projektu zkopiruj:
SPUSTIT_BUILD2ED_REAL_WAV_IRG_AUDIT.cmd
_EMU10_BUILD2ED_PAYLOAD
README_BUILD2ED_PRESNY_POSTUP.txt

CO SPUSTIT
----------
V koreni projektu dvakrat klikni na:
SPUSTIT_BUILD2ED_REAL_WAV_IRG_AUDIT.cmd

CO SE ZMENI
-----------
Meni se jen:
app\src\main\assets\emu\index.html

Nesaha se na:
- Java cast
- XEX picker
- CSAVE zachyt SEROUT
- ikony
- hlavni menu
- ROM data

KODY JSOU STEJNE
----------------
BASIC/Altirra porovnavaci kody nejsou zmenene.

PROC BUILD2ED
-------------
Predchozi log ukazal, ze kratky interni CLOAD nebyl dukaz realne WAV kazety.
BUILD2ED proto:
- prida ROM CRC32 audit do logu
- prida CSAVE SEROUT audit: delka a 132B recordy
- prepisuje WAV export na 48 kHz / 600 baud / kontinualni FSK
- pridava MARK/IRG pauzu mezi 132B recordy
- vypina stary LAST-CSAVE RAW shortcut
- log uz nema tvrdit, ze realna kazeta je hotova bez realneho 130XE testu

TEST PO APK
-----------
1) POWER BASIC
   V LOGu hledej: ROM AUDIT BUILD2ED

2) Vloz svuj dlouhy kod pres BASIC/TBXL TXT
   RUN over podle sebe.

3) Dej CSAVE
   V LOGu hledej:
   CSAVE AUDIT BUILD2ED
   SAVE WAV BUILD2ED

4) V Downloads/AtariHelp vezmi novy WAV a zkus ho na realnem Atari 130XE.

5) Pokud realny Atari hodi ERROR 138:
   Posli zpatky LOG z appky a ten WAV.

DULEZITE
--------
Tohle neni slib zazraku. Je to ferova oprava smerem k realne kazete:
- zadny Decathlon graficky pokus
- zadne rozsypani XEX
- zadne fake tvrzeni o realnem CLOAD
- oprava WAV tvaru podle chyby dlouheho kodu
