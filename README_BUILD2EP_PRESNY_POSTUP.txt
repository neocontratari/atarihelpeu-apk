BUILD2EP - PRESNY POSTUP

KODY JSOU STEJNE

Co se meni:
- jen app/src/main/assets/emu/index.html

Co se NESMI menit:
- CSAVE WAV export zustava, protoze ho otestoval realny Atari 130XE
- XEX zustava
- Java zustava
- ROM zustava
- UI zustava

Postup:
1. V Total Commanderu vlevo otevri koren projektu:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2. Vpravo otevri tento ZIP a slozku build2ep_cmd_installer.

3. Do leveho korene projektu zkopiruj F5 tyto 2 veci:
   SPUSTIT_BUILD2EP_REAL_ATARI_WAV_EXACT_SERIN.cmd
   _EMU10_BUILD2EP_PAYLOAD

4. Vlevo v koreni projektu spust:
   SPUSTIT_BUILD2EP_REAL_ATARI_WAV_EXACT_SERIN.cmd

5. GitHub Desktop:
   Summary: BUILD2EP exact SERIN no skip
   Commit to main
   Push origin

6. WWW / Actions vyrobi APK.

Log po startu musi obsahovat:
AtariHelp.eu EMU-10 BUILD2EP_REAL_ATARI_WAV_EXACT_SERIN_NO_SKIP pripraven.

Pri CLOAD hledej:
WAV CLOAD BUILD2EP: EXACT SERIN
NEPRESKAKUJU $55 $55

Jestli se objevi ERROR 138, posli log.
