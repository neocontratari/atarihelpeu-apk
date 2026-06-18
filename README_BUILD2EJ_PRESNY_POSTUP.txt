ATARIHELP.EU EMU-10 BUILD2EJ_ALTIRRA_OK_WAV_AUTO_INSERT

KODY JSOU STEJNE.

Duvod opravy:
- WAV z BUILD2EI podle Reneho nacetla Altirra.
- Problem v EMU nebyl WAV export, ale mechanika: PLAY hlasil "neni vlozena kazeta" a nebyl slyset zvuk.
- BUILD2EJ po realnem CSAVE automaticky vlozi posledni vytvoreny WAV do virtualni XC12.
- CLOAD v emulatoru ted testuj takto: po CSAVE napis CLOAD a stiskni PLAY. Zvuk musi byt slyset.

Zasah:
- meni se jen app\src\main\assets\emu\index.html
- Java beze zmeny
- ROM beze zmeny
- XEX beze zmeny
- BASIC/Altirra srovnavaci kody beze zmeny

Postup:
1) V Total Commanderu vlevo otevri:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
2) Z tohoto installeru zkopiruj do korene projektu:
   SPUSTIT_BUILD2EJ_ALTIRRA_OK_WAV_AUTO_INSERT.cmd
   _EMU10_BUILD2EJ_PAYLOAD
   README_BUILD2EJ_PRESNY_POSTUP.txt
3) Vlevo v koreni projektu spusť:
   SPUSTIT_BUILD2EJ_ALTIRRA_OK_WAV_AUTO_INSERT.cmd
4) GitHub Desktop:
   Summary: BUILD2EJ Altirra OK WAV auto insert
   Commit to main
   Push origin
5) WWW/GitHub Actions vyrobi APK.

Test:
1) POWER BASIC.
2) Vloz svuj dlouhy TXT kod.
3) CSAVE - po konci ma log napsat BUILD2EJ AUTO INSERT.
4) NEW, ENTER.
5) CLOAD, ENTER.
6) PLAY CLOAD.

Spravne v LOGu:
AtariHelp.eu EMU-10 BUILD2EJ_ALTIRRA_OK_WAV_AUTO_INSERT pripraven.
BUILD2EJ AUTO INSERT: vkladam posledni CSAVE WAV do virtualni XC12 mechaniky
VLOZIT KAZETU: WAV AtariHelp_CSAVE...
KAZETA PLAY BUILD2EJ: spoustim slyšitelny WAV/CAS audio monitor

Pokud selze:
Posli LOG a napis jestli:
- nebylo slyset audio
- porad ERROR 138
- nenapsalo BUILD2EJ AUTO INSERT
- Altirra nacita WAV, ale EMU ne
