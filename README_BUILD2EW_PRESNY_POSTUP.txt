BUILD2EW_RECORD5_BUFFER_FLAG_FIX

KODY JSOU STEJNE

Zaklad: posledni funkcni EJ/EV cesta. CSAVE WAV export se NEMENI, protoze funguje v Altirre i na realnem Atari 130XE.
Meni se pouze:
app\src\main\assets\emu\index.html

Co opravuje:
- BUILD2EV podle logu nacetl record 1-4 OK.
- Potom visel na recordu 5 pri PC=$F057 az do konce zvuku.
- BUILD2EW uz neceka jen na uzke PC okno.
- Dalsi C: record pusti podle OS cassette buffer flagu, PC gate $EF00-$F0FF a pojistky proti nekonecnemu cekani.
- Novy leader pres obraz se nepousti.

Postup:
1. V Total Commanderu vlevo otevri koren projektu:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
2. Zkopiruj tam:
   SPUSTIT_BUILD2EW_RECORD5_BUFFER_FLAG_FIX.cmd
   _EMU10_BUILD2EW_PAYLOAD
3. Spust CMD vlevo v koreni projektu.
4. GitHub Desktop:
   Summary: BUILD2EW record5 buffer flag fix
   Commit to main
   Push origin

V logu hledej:
AtariHelp.eu EMU-10 BUILD2EW_CLOAD_RECORD5_BUFFER_FLAG_FIX pripraven.
CLOAD SERVICE BUILD2EW: record ... ok=ANO
CLOAD SERVICE BUILD2EW: ROM/BASIC chce dalsi C: record ... flags=... timeout=...
