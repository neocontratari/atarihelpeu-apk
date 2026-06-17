ATARIHELP.EU EMU-10 BUILD2EE_CLOAD_SEROUT_TIMELINE_NO_CMD
===========================================================

KODY JSOU STEJNE.

Tento balicek je BEZ CMD. Nespousti se zadny prikaz.
Meni se jen jeden soubor:

app\src\main\assets\emu\index.html

Cil v tvem projektu podle screenshotu:

C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\app\src\main\assets\emu\index.html

CO SE ZMENILO:
- Nejde o Decathlon grafiku.
- Nejde jen o audit.
- Pri CSAVE se nově ukladaji i CPU cykly kazdeho skutecneho POKEY SEROUT bajtu.
- WAV export BUILD2EE se sklada podle realnych rozestupu tech SEROUT bajtu.
- Neřeže data naslepo po 132 B jako BUILD2ED.
- Nesaha na Java cast, XEX picker, CSAVE zachyt, UI ikony ani hlavni menu.
- ROM audit zustava, ale hlavni oprava je SEROUT timeline WAV/CLOAD cast.

PRESNY POSTUP BEZ CMD V TOTAL COMMANDERU:

1) Vlevo otevri projekt:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\

2) Vlevo pak otevri presnou slozku:
   app\src\main\assets\emu\

3) Vpravo otevri ZIP BUILD2EE a jdi do:
   app\src\main\assets\emu\

4) Vpravo oznac soubor:
   index.html

5) Dej F5 Kopirovat doleva.

6) Potvrd PREPSAT / NAHRADIT soubor v cili.

7) Otevri GitHub Desktop.
   Musi ukazat zmenu:
   app/src/main/assets/emu/index.html

8) Summary:
   BUILD2EE CLOAD SEROUT timeline

9) Dej:
   Commit to main
   Push origin

10) Pak WWW/GitHub Actions udela APK jako predtim.

CO TESTOVAT:
- Ne kratky demo kod.
- Testuj svuj dlouhy kod.
- Testuj XEX hry, ktere fungovaly.
- Testuj CSAVE.
- Po CSAVE v logu hledej:
  CSAVE AUDIT BUILD2EE
  SAVE WAV BUILD2EE
  timelineAware=ANO
  lead55=
  goodDelta=
  badDelta=

KDYZ REALNE ATARI 130XE ZASE HODI ERROR 138:
- posli LOG z EMU
- posli ulozeny WAV
- napis, jestli zvuk je rozjety hned od zacatku nebo az po leaderu

DULEZITE:
BUILD2EE neni slib, ze realny Atari WAV je hotovy. Je to skutecna kodova oprava proti BUILD2ED:
zachovat casovani SEROUT bajtu misto umeleho record-aware rezu.
