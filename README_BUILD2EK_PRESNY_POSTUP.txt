ATARIHELP.EU EMU-10 BUILD2EK_DIRECT_WAV_TO_SKSTAT_CLOAD

KODY JSOU STEJNE.

Proc vznikl BUILD2EK:
- WAV z BUILD2EJ nacetla Altirra, tak se NESMI dal menit WAV export.
- EMU stale padal ERROR 138, protoze CLOAD v emu nebral stejny WAV jako audio hrany.
- BUILD2EK meni jen CLOAD vstup: vlozeny WAV jde primo jako PCM hrany na SKSTAT bit4.
- Vypina prakticky pro vlastni WAV cestu decode->hotove SERIN bajty.
- Zadny RAM inject, zadny TXT shortcut.

Meni se jen:
app\src\main\assets\emu\index.html

Nemeni se:
Java, XEX, ROM data, UI, Decathlon grafika, BASIC/Altirra srovnavaci kody.

PRESNY POSTUP:
1) V Total Commanderu vlevo otevri:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2) Vpravo otevri ZIP BUILD2EK a slozku build2ek_cmd_installer.

3) Zkopiruj vlevo do korene projektu tyto 3 veci:
   SPUSTIT_BUILD2EK_DIRECT_WAV_SKSTAT.cmd
   _EMU10_BUILD2EK_PAYLOAD
   README_BUILD2EK_PRESNY_POSTUP.txt

4) Vlevo spust:
   SPUSTIT_BUILD2EK_DIRECT_WAV_SKSTAT.cmd

5) GitHub Desktop:
   Summary: BUILD2EK direct WAV to SKSTAT CLOAD
   Commit to main
   Push origin

6) WWW / GitHub Actions vyrobi APK.

TEST:
- POWER BASIC
- Vloz svuj dlouhy kod
- CSAVE
- po CSAVE se WAV auto vlozi
- NEW
- CLOAD
- PLAY

V LOGu musi byt:
AtariHelp.eu EMU-10 BUILD2EK_DIRECT_WAV_TO_SKSTAT_CLOAD pripraven.
WAV DIRECT CLOAD BUILD2EK
CLOAD WAV DIRECT BUILD2EK

Kdyz je tam CLOAD SERIAL / SERIN jako hlavni cesta, je to spatne nebo spadl direct WAV parser.
