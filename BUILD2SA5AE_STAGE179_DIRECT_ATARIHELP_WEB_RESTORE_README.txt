AtariHelp EMU10 BUILD2SA5AE / Stage179

Base:
- Stage164 local 130XE XEX/ATR picker behavior kept.
- Stage176/177 ZIP routing kept.
- Stage178 local XEX bridge kept in code only as fallback, not used by NET HRY.

Fix in this overlay:
- AtariHelp/WEDOS are no longer forced through provider relay.
- NET HRY opens the real website directly: https://atarihelp.eu/?page_id=207
- AtariHelp direct browsing is not rate-limited by the old anti-ban guard.
- WebView still catches ZIP/XEX/ATR game navigation and routes it into the emulator.
- Atari web ZIP content is inspected and .xex/.com/.exe payload is sent to 130XE, with .atr as fallback.
- Added log markers BUILD2SA5AE WEBVIEW_DIRECT_WEB and BUILD2SA5AE ATARIHELP_DIRECT_ALLOWED.

Not changed:
- No Sega emulator core/assets edits.
- No 130XE emulator core/assets edits.
- No local mobile XEX picker rewrite from later broken stages.
- No BIOS files.
