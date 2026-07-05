BUILD2SA5Y / STAGE173 - protect local Atari picker, remove WebView callback dependency

Problem:
- Stage171/172 made Atari loading depend on evaluateJavascript callback readiness.
- In Nox/WebView this can stall the local Atari game path too: the 130XE page opens and stays at READY, with no "Prijimam z webu" / "NAHRAJ XEX" in the 130XE log.

Fix:
- Remove callback-dependent Atari injection.
- Keep the Atari payload queue and ZIP content detection.
- After the 130XE page is loaded, wait a fixed 750 ms and send AHRECV_BEGIN/PART/END fire-and-forget again.
- Use smaller 65536-character base64 chunks.
- Local mobile picks and web Atari downloads use this safer legacy transfer path.

Log markers:
- BUILD2SA5Y EMU130_QUEUE
- BUILD2SA5Y EMU130_INJECT_NO_CALLBACK
- BUILD2SA5Y EMU130_INJECT_SEND_LEGACY_SAFE
- BUILD2SA5Y EMU130_INJECT_SENT_LEGACY_SAFE

Scope:
- MainActivity.java only.
- No Sega asset edits.
- No 130XE asset/core edits.
- No PS1 asset/core edits.
- No BIOS files included.
