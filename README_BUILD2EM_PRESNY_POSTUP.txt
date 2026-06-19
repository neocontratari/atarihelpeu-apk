ATARIHELP.EU EMU-10 BUILD2EM_ALTIRRA_OK_WAV_PCM_SKSTAT

KODY JSOU STEJNE.

Duvod opravy:
- BUILD2EK rozbil boot/emulator. BUILD2EK se zahazuje.
- BUILD2EJ bootoval, tlacitka fungovala, CSAVE fungovalo a WAV byl slyset.
- Rene ukazal, ze stejny WAV nacte Altirra. Proto WAV export NEMENIT.
- Problem je CLOAD vstup v EMU: BUILD2EJ sel cestou WAV decode -> hotove SERIN bajty a to skoncilo ERROR 138.
- BUILD2EM je zalozen na BOOT OK BUILD2EJ, ale CLOAD z WAV pousti pres existujici PCM->SKSTAT bit4 cestu. Zadny decode->SERIN shortcut.

Zasah:
- meni se jen app\src\main\assets\emu\index.html
- Java beze zmeny
- ROM beze zmeny
- XEX beze zmeny
- UI/keyboard/XC12 obraz beze zmeny
- WAV export beze zmeny
- CSAVE beze zmeny

Postup:
1) V Total Commanderu vlevo otevri:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
2) Z tohoto installeru zkopiruj do korene projektu:
   SPUSTIT_BUILD2EM_ALTIRRA_OK_PCM_SKSTAT.cmd
   _EMU10_BUILD2EM_PAYLOAD
   README_BUILD2EM_PRESNY_POSTUP.txt
3) Vlevo v koreni projektu spusť:
   SPUSTIT_BUILD2EM_ALTIRRA_OK_PCM_SKSTAT.cmd
4) GitHub Desktop:
   Summary: BUILD2EM Altirra OK PCM SKSTAT
   Commit to main
   Push origin
5) WWW/GitHub Actions vyrobi APK.

Test:
1) POWER BASIC musi normalne najet. Pokud ne, neposilej CLOAD test, posli screenshot/log.
2) Vloz svuj dlouhy TXT kod.
3) CSAVE.
4) Po CSAVE musi log ukazat BUILD2EM AUTO INSERT.
5) NEW.
6) CLOAD.
7) PLAY.

Spravne v LOGu:
AtariHelp.eu EMU-10 BUILD2EM_ALTIRRA_OK_WAV_PCM_SKSTAT pripraven.
BUILD2EM AUTO INSERT: vkladam posledni CSAVE WAV
KAZETA PLAY BUILD2EM: spoustim slyšitelny WAV/CAS audio monitor
WAV PCM REAL CLOAD BUILD2EM: ALTIRRA_OK WAV posilam primo na SKSTAT bit4

Nesmí tam být hlavní CLOAD cesta:
CLOAD SERIAL
SERIN predano
SERIN byte ... drzim

Pokud selze:
Posli LOG. Hlavne radky s BUILD2EM, WAV PCM REAL CLOAD, MOTOR, SKSTAT a ERROR 138.
