AtariHelp EMU10 BUILD2SA5AC / Stage177

Base:
- Stage164 local 130XE XEX/ATR picker behavior kept.
- Stage175/176 provider relay and Sega ZIP route kept.

Fix in this overlay:
- AtariHelp XEX page ZIP clicks are now guarded at main-frame WebView navigation level.
- If WebView tries to open .zip/.xex/.atr as a page, MainActivity intercepts it and routes it to downloadAndRun().
- Atari web ZIP content is inspected and .xex/.com/.exe payload is sent to 130XE, with .atr as fallback.
- Added clear log markers: BUILD2SA5AC AHNET_RUN_GAME_URL, HANDLE_GAME_URL, MAINFRAME_GAME_NAV, WEB_GAME_DOWNLOADED, ZIP_CONTAINS_ATARI.

Not changed:
- No Sega emulator core/assets edits.
- No 130XE emulator core/assets edits.
- No local mobile XEX picker rewrite from later broken stages.
- No BIOS files.
