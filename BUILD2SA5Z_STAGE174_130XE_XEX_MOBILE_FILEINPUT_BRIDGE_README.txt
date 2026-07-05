BUILD2SA5Z / STAGE174 - 130XE XEX MOBIL file-input bridge

Problem:
- Built-in Turbo BASIC boots, so the 130XE core and bootXex path are alive.
- XEX MOBIL / ATR DISK from the mobile picker does not reach bootAny/AHRECV.
- The saved 130XE log has no "XEX MOBIL: vybran", no "Prijimam z webu", and no "NAHRAJ XEX".

Fix:
- Keep MainActivity-only scope.
- Inject a small bridge into the 130XE page after load.
- The bridge captures dockXex and dockAtr on pointerup/click before the original safeTap handler.
- It uses the page's hidden file input directly, reads the selected file with FileReader, then sends it to existing AHRECV_BEGIN/PART/END.
- This bypasses the Android AHPICK game path for XEX MOBIL while leaving Turbo BASIC and the 130XE core unchanged.

Log markers in 130XE log:
- BUILD2SA5Z local mobile file picker bridge active
- BUILD2SA5Z XEX MOBIL: selected ...
- BUILD2SA5Z XEX MOBIL: read ... B
- BUILD2SA5Z AHRECV_END ... bytes=... parts=...

Scope:
- MainActivity.java only.
- No Sega asset edits.
- No 130XE asset/core edits.
- No PS1 asset/core edits.
- No BIOS files included.
