BUILD2EX PURE POKEY CASSETTE RX

KODY JSOU STEJNE.

Zaklad: posledni funkcni EJ/BZ UI.
Nedotceno: CSAVE WAV export, XEX, ROM, Java, UI, klavesnice, joystick, grafika.
Meni se jen: app/src/main/assets/emu/index.html

Co je vyhozene:
- OS-buffer CLOAD lepeni ES/ET/EU/EV/EW
- record gate podle BASIC/OS PC hacku
- zapisovani dekodovanych bajtu do OS cassette bufferu

Co je nove:
- CLOAD jde pres POKEY cassette RX stream
- leader/sync jde pres SKSTAT bit4
- data jdou pres SERIN v case pasky
- zadny RAM inject, zadny OS-buffer inject

Postup:
1. V Total Commanderu vlevo otevri koren projektu:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
2. Z tohoto installeru zkopiruj do korene projektu:
   SPUSTIT_BUILD2EX_PURE_POKEY_CASSETTE_RX.cmd
   _EMU10_BUILD2EX_PAYLOAD
3. Spust vlevo v projektu CMD.
4. GitHub Desktop:
   Summary: BUILD2EX pure POKEY cassette RX
   Commit to main
   Push origin
5. WWW / Actions vyrobi APK.

V logu hledej:
AtariHelp.eu EMU-10 BUILD2EX_PURE_POKEY_CASSETTE_RX pripraven.
WAV CLOAD BUILD2EX
STREAM_RECORD_LEADER
STREAM_SERIN_ARM
STREAM_RECORD_SENT

Pokud selze, poslat LOG.
