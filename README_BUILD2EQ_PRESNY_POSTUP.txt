BUILD2EQ RECORD SERIN CLOAD CLEAN - PRESNY POSTUP

1) V Total Commanderu vlevo nech projekt:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2) Z tohoto ZIPu zkopiruj do LEVEHO projektu pres F5 tyto 2 veci:
   SPUSTIT_BUILD2EQ_RECORD_SERIN_CLOAD.cmd
   _EMU10_BUILD2EQ_PAYLOAD

3) VLEVO v projektu spust:
   SPUSTIT_BUILD2EQ_RECORD_SERIN_CLOAD.cmd

4) Musi napsat:
   HOTOVO: BUILD2EQ installed.

5) GitHub Desktop:
   Summary: BUILD2EQ record serin cload clean
   Commit to main
   Push origin

Co se meni:
- pouze app/src/main/assets/emu/index.html
- CSAVE WAV export se nemeni
- XEX se nemeni
- ROM se nemeni
- UI/klavesnice/joystick se nemeni
- Java se nemeni

Co oprava dela:
- maze spatnou EP/EO CLOAD vetve z aktivniho index.html
- CLOAD z vlastniho WAV jde po 132B Atari recordech
- pred kazdym recordem je samostatna zavadeci/mark faze
- do POKEY SERIN jde control+128 data+checksum, ne sync $55 $55 jako data
- nepousti druhou CLOAD frontu paralelne

KODY JSOU STEJNE.
