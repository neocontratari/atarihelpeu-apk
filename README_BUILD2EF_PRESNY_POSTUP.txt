ATARIHELP.EU EMU-10 BUILD2EF - PRESNY POSTUP
============================================

UCEL:
- Neopravuje Decathlon grafiku.
- Nehrabe do Java casti, XEX pickeru, hlavniho menu ani ikon.
- Tvrdym prepisem meni jen: app/src/main/assets/emu/index.html
- Cisti jen build cache a stare instalacni payloady.

KODY JSOU STEJNE.

PROC BUILD2EF:
- Log BUILD2EE ukazal problem: SAVE WAV mel jen cca 1 000 612 B pri 2904 B SEROUT.
- To je spatne, protoze realna 600 baud kazeta musi mit delku podle 60 B/s + leader + IRG.
- BUILD2EF proto vyhazuje SEROUT timeline z WAV exportu.
- Nove dela standardni Atari WAV: 48 kHz, 600 baud, 5327/3995 Hz, 132B rekordy, dlouhy MARK leader, short IRG.
- Opravuje i FUJI CAS strukturu.

PRESNY POSTUP V TOTAL COMMANDERU:
1) Vlevo otevri projekt:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2) Vpravo otevri ZIP:
   AtariHelp_eu_EMU10_BUILD2EF_HARD_CLOAD_WAV_STANDARD_CMD.zip

3) V ZIPu otevri slozku:
   build2ef_cmd_installer

4) Zkopiruj vlevo do korene projektu tyto 3 veci:
   SPUSTIT_BUILD2EF_TVRDA_CLOAD_WAV_OPRAVA.cmd
   _EMU10_BUILD2EF_PAYLOAD
   README_BUILD2EF_PRESNY_POSTUP.txt

5) Vlevo v koreni projektu spust:
   SPUSTIT_BUILD2EF_TVRDA_CLOAD_WAV_OPRAVA.cmd

6) Po HOTOVO otevri GitHub Desktop:
   Summary: BUILD2EF hard CLOAD WAV standard
   Commit to main
   Push origin

7) WWW/GitHub Actions udela APK jako driv.

TEST:
1) Spust novou APK.
2) V LOGu musi byt:
   AtariHelp.eu EMU-10 BUILD2EF_HARD_CLOAD_WAV_STANDARD pripraven.
3) Vloz svuj dlouhy BASIC kod.
4) Udelej CSAVE.
5) V LOGu musi byt:
   CSAVE AUDIT BUILD2EF
   SAVE WAV BUILD2EF
   recordAware=ANO
   timelineAware=NE
   pre=8s
   amp=16%
6) WAV z Downloads/AtariHelp zkus na realnem Atari 130XE.

KDYZ TO SELZE:
Posli:
- LOG z EMU-10
- WAV, ktery BUILD2EF ulozil
- napiš: ERROR 138 / ERROR 143 / ERROR 21 / zvuk prebuzeny / ticho / LIST spatny
