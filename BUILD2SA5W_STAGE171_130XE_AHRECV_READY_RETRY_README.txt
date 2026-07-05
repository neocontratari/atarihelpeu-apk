BUILD2SA5W / STAGE171 - 130XE waits for AHRECV before injecting Atari games

Problem:
- Atari XEX/ATR from the relayed AtariHelp page opened the 130XE screen, but the game stayed at READY.
- The likely failure was timing: Android sent AHRECV_BEGIN/PART/END immediately on WebView onPageFinished and cleared pendingGame even when the 130XE JavaScript receiver was not ready yet.

Fix:
- Atari web/local games now go through queueAtariGameFor130xe().
- pendingGame is kept until the 130XE page confirms these functions exist:
  AHRECV_BEGIN, AHRECV_PART, AHRECV_END.
- The injector retries for up to 80 attempts with 250 ms spacing.
- pendingGame is cleared only after the receiver is ready and injectGame() is actually called.
- Stage170 content detection stays intact:
  Sega ZIP -> Sega route.
  Atari ZIP with .xex/.exe/.com/.atr -> unzip inner payload and send to 130XE.

Log markers to check:
- BUILD2SA5W EMU130_QUEUE
- BUILD2SA5W EMU130_INJECT_WAIT
- BUILD2SA5W EMU130_INJECT_READY
- BUILD2SA5W EMU130_INJECT_SEND
- BUILD2SA5W EMU130_INJECT_SEND_ERROR

Scope:
- MainActivity.java only.
- No Sega asset edits.
- No 130XE asset edits.
- No PS1 asset/core edits.
- No BIOS files included.
