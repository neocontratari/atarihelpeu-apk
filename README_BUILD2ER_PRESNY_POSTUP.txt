BUILD2ER RECORD SERIN TIMING FIX

KODY JSOU STEJNE

Zmenen soubor:
app\src\main\assets\emu\index.html

Nezmeneno:
CSAVE WAV export, Java, XEX, ROM, UI, klavesnice, joystick.

Duvod:
CSAVE WAV funguje v Altirre i na realnem Atari 130XE. Chyba je v EMU CLOAD prijmu.
BUILD2EQ dokazal prvni record pripravit, ale kolem 128/130 B skoncil ERROR 138.
BUILD2ER upravuje timing POKEY SERIN uvnitr recordu: po prvnim bajtu recordu uz dalsi bajty tecou pevnym 600baud casovanim a necekaji na spatny PC/timer gate.

Postup:
1) V Total Commanderu vlevo otevri koren projektu:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
2) Vpravo otevri ZIP a slozku build2er_cmd_installer.
3) Zkopiruj vlevo do korene projektu:
   SPUSTIT_BUILD2ER_RECORD_SERIN_TIMING_FIX.cmd
   _EMU10_BUILD2ER_PAYLOAD
4) Vlevo spust CMD.
5) GitHub Desktop:
   Summary: BUILD2ER record serin timing fix
   Commit to main
   Push origin
6) WWW / GitHub Actions vytvori APK.

Log po startu:
AtariHelp.eu EMU-10 BUILD2ER_RECORD_SERIN_TIMING_FIX pripraven.

Log pri CLOAD:
WAV CLOAD BUILD2ER
BUILD2ER RECORD_AWARE_SERIN_TIMING
RECORD_SERIN_BLOCK_SENT
RECORD_SERIN_READ_DONE
