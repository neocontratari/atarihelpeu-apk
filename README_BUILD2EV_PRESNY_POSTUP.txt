ATARIHELP.EU EMU-10 BUILD2EV CLOAD RECORD3 GATE FIX

KODY JSOU STEJNE

Zmenen jen:
app\src\main\assets\emu\index.html

CSAVE WAV export, XEX, ROM, Java, UI, klavesnice a joystick zustavaji beze zmeny.

Oprava proti BUILD2EU:
BUILD2EU po recordu 2 cekal na record 3 pouze v uzkem C: read PC gate.
Log ukazal, ze ROM/BASIC realne ceka na dalsi record v BASIC/OS adresach kolem
$B939 / $BA9B / $BCD4 / $F220 / $F30B.
BUILD2EV proto v tomto stavu neposila dalsi leader pres obraz, ale predava dalsi
record primo do OS cassette bufferu ve chvili, kdy ROM/BASIC ceka C: read.

Postup:
1. V Total Commanderu vlevo otevri koren projektu:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
2. Zkopiruj sem:
   SPUSTIT_BUILD2EV_CLOAD_RECORD3_GATE_FIX.cmd
   _EMU10_BUILD2EV_PAYLOAD
3. Spust CMD.
4. GitHub Desktop:
   Summary: BUILD2EV CLOAD record3 gate fix
   Commit to main
   Push origin
5. WWW / Actions vyrobi APK.

V logu hledej:
AtariHelp.eu EMU-10 BUILD2EV_CLOAD_RECORD3_GATE_FIX pripraven.
CLOAD SERVICE BUILD2EV: ROM/BASIC chce dalsi C: record ... DIRECT OS BUFFER
CLOAD SERVICE BUILD2EV: record ... ok=ANO
