ATARIHELP.EU EMU-10 BUILD2EU CLOAD WAIT NEXT RECORD FIX

KODY JSOU STEJNE

ZAKLAD:
- BUILD2ET/EJ funkcni appka zustava.
- CSAVE WAV export se NEMENI, protoze funguje v Altirre i na realnem Atari 130XE.
- XEX, ROM, Java, UI, klavesnice, joystick se NEMENI.
- Meni se jen app/src/main/assets/emu/index.html.

OPRAVA:
BUILD2ET uz mel spravny checksum recordu, ale po recordu 2 zacal drzet leader pres BASIC/editor stav.
BUILD2EU po zapsani jednoho OS-buffer recordu CEKA, az ROM znovu vejde do skutecneho C: read stavu,
a teprve potom spusti dalsi record. Tim se nema skakat do jine obrazovky.

POSTUP:
1) V Total Commanderu vlevo otevri koren projektu:
   C:\Users
eocontr\Documents\GitHub\atarihelpeu-apk\
2) Z tohoto baliku zkopiruj vlevo:
   SPUSTIT_BUILD2EU_CLOAD_WAIT_NEXT_RECORD.cmd
   _EMU10_BUILD2EU_PAYLOAD
3) Vlevo spust:
   SPUSTIT_BUILD2EU_CLOAD_WAIT_NEXT_RECORD.cmd
4) GitHub Desktop:
   Summary: BUILD2EU CLOAD wait next record
   Commit to main
   Push origin
5) WWW / Actions vyrobi APK.

LOG:
Po startu hledej:
AtariHelp.eu EMU-10 BUILD2EU_CLOAD_WAIT_NEXT_RECORD_FIX pripraven.

Pri CLOAD hledej:
CLOAD SERVICE BUILD2EU: record ... ok=ANO
CLOAD SERVICE BUILD2EU: record ... hotovy, CEKAM na dalsi realny C: read stav
CLOAD SERVICE BUILD2EU: ROM znovu chce C: read
