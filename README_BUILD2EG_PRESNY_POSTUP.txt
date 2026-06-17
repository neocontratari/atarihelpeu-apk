ATARIHELP.EU EMU-10 BUILD2EG_REALSAFE_WAV_NO_MONITOR
====================================================

KODY JSOU STEJNE.

PROC TENHLE BUILD:
- Realny Atari 130XE dal ERROR 138.
- Uzivatel slysel na konci "skakani" a mel podezreni na michani dvou zvuku.
- Log BUILD2EF ukazal 2904 B SEROUT = 22 x 132B, ale realny WAV porad neni dobry.
- BUILD2EG proto NELEPI Decathlon ani grafiku. Resi jen kazetovy export.

CO SE MENI:
- Meni se pouze app/src/main/assets/emu/index.html.
- Java, XEX, CSAVE zachyt, UI, ROM data se nemení.
- Live WebAudio monitor pri CSAVE je vypnuty, aby se nemohl smichat druhy zvuk.
- FUJI CAS se uklada po 132B DATA rekordech.
- WAV export je REALSAFE:
  48 kHz, 600 baud, MARK 5327 Hz, SPACE 3995 Hz,
  12 s leader, 0.75 s IRG mezi rekordy, 4 s tail, amp 20 %.
- Audit kontroluje 132B rekordy a checksumy. Pokud checksumBad=0, SEROUT data z ROM/POKEY vypadaji jako platna C: kazeta.

PRESNY POSTUP V TOTAL COMMANDERU:
1) VLEVO nech projekt:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2) VPRAVO otevri tento ZIP a slozku:
   build2eg_cmd_installer

3) Zkopiruj VLEVO do korene projektu tyto veci:
   SPUSTIT_BUILD2EG_REALSAFE_WAV_NO_MONITOR.cmd
   _EMU10_BUILD2EG_PAYLOAD
   README_BUILD2EG_PRESNY_POSTUP.txt

4) VLEVO spust:
   SPUSTIT_BUILD2EG_REALSAFE_WAV_NO_MONITOR.cmd

5) V GitHub Desktop:
   Summary: BUILD2EG realsafe wav no monitor
   Commit to main
   Push origin

6) WWW / GitHub Actions vyrobi APK jako predtim.

CO HLEDAT V LOGU:
- AtariHelp.eu EMU-10 BUILD2EG_REALSAFE_WAV_NO_MONITOR pripraven.
- CSAVE MONITOR BUILD2EG: live WebAudio monitor je VYPNUTY.
- CSAVE AUDIT BUILD2EG: ... checksumBad=0
- SAVE WAV BUILD2EG: REALSAFE export ...

TEST:
- Pouzij svuj dlouhy kod.
- Udelej CSAVE.
- Vem novy WAV z Downloads/AtariHelp.
- Prehraj ho realnemu Atari 130XE.
- Kdyz bude ERROR 138, posli LOG + WAV a napis cas, kdy zvuk zacal skakat.

POZNAMKA:
Konec s mnoha nulami v EOF recordu muze znit jako pravidelne "skakani".
To samo o sobe neni druhy zvuk. Dulezite je, jestli realny Atari nacte EOF bez ERROR 138.
