BUILD2EO REAL ATARI WAV CLEAN FSK CLOAD

KODY JSOU STEJNE

ZAKLAD:
- zachovana funkcni appka / BUILD2EN cesta
- CSAVE WAV export nemenim, protoze WAV uz nacetla Altirra i realny Atari 130XE
- XEX, Java, ROM, UI, klavesnice, joystick se nemeni

OPRAVA:
- meni se jen app/src/main/assets/emu/index.html
- CLOAD v emulatoru uz nepouziva hruby PCM zero-crossing z WAV
- CLOAD vlastniho AtariHelp CSAVE WAV: WAV se dekoduje jen na 132B Atari kazetove rekordy
- potom se do ROM C: handleru posila cisty 600 baud FSK tvar pres SKSTAT bit4
- zadny RAM inject, zadny fake BASIC, zadny SERIN shortcut pro vlastni CSAVE WAV

PRESNY POSTUP:
1. V Total Commanderu vlevo otevri:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2. Vpravo otevri ZIP a slozku build2eo_cmd_installer.

3. Zkopiruj F5 vlevo do korene projektu tyto 2 veci:
   SPUSTIT_BUILD2EO_REAL_ATARI_WAV_CLEAN_FSK.cmd
   _EMU10_BUILD2EO_PAYLOAD

4. Vlevo spust:
   SPUSTIT_BUILD2EO_REAL_ATARI_WAV_CLEAN_FSK.cmd

5. GitHub Desktop:
   Summary: BUILD2EO real Atari wav clean FSK cload
   Commit to main
   Push origin

6. WWW / Actions vyrobi APK.

TEST:
- POWER BASIC
- vloz svuj dlouhy kod
- CSAVE
- realny WAV export nemenim
- po CSAVE dej NEW, CLOAD, PLAY

V LOGU MUSI BYT:
AtariHelp.eu EMU-10 BUILD2EO_REAL_ATARI_WAV_CLEAN_FSK_CLOAD pripraven.
WAV CLOAD BUILD2EO: CLEAN FSK->SKSTAT pripraveno

V LOGU NESMI BYT jako hlavni cesta pro vlastni CSAVE WAV:
WAV PCM CLOAD BUILD2EN
SERIN predano / HOLD_SERIN
