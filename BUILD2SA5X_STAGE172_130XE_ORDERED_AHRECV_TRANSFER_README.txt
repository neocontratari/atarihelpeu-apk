BUILD2SA5X / STAGE172 - ordered 130XE AHRECV transfer

Problem:
- Atari web games opened the 130XE screen but stayed at READY/reset instead of booting the XEX/ATR.
- Stage171 waited for AHRECV_BEGIN/PART/END, but the actual payload was still sent as separate asynchronous evaluateJavascript calls.

Fix:
- Keep Stage171 receiver readiness wait.
- Send the payload in strict callback order:
  AHRECV_BEGIN -> callback OK -> AHRECV_PART chunk -> callback OK -> next chunk -> AHRECV_END.
- Reduce chunk size to 65536 base64 chars for safer Android WebView delivery.
- Add detailed native markers so the next log can show whether the transfer reaches BEGIN, PART and END.

Log markers to check:
- BUILD2SA5W EMU130_QUEUE
- BUILD2SA5W EMU130_INJECT_READY
- BUILD2SA5X EMU130_INJECT_SEND_ORDERED
- BUILD2SA5X EMU130_BEGIN_OK / BEGIN_FAIL
- BUILD2SA5X EMU130_PART_OK / PART_FAIL
- BUILD2SA5X EMU130_END_OK / END_FAIL

Scope:
- MainActivity.java only.
- No Sega asset edits.
- No 130XE asset/core edits.
- No PS1 asset/core edits.
- No BIOS files included.
