AtariHelp EMU10 BUILD2SA5AD / Stage178

Base:
- Stage164 local 130XE XEX/ATR picker behavior kept.
- Stage175/176/177 provider relay and Sega ZIP route kept.

Fix in this overlay:
- NET HRY opens a local Atari XEX bridge inside the app.
- The bridge contains direct ZIP links to AtariHelp XEX games from page_id=21.
- Each game click calls AHNET.runGameUrl(url) directly, so the click cannot be lost by WebView/WordPress navigation.
- The ZIP still downloads from atarihelp.eu through the existing provider relay path.
- Atari web ZIP content is inspected and .xex/.com/.exe payload is sent to 130XE, with .atr as fallback.
- Added log marker BUILD2SA5AD ATARI_NET_BRIDGE_OPEN.

Not changed:
- No Sega emulator core/assets edits.
- No 130XE emulator core/assets edits.
- No local mobile XEX picker rewrite from later broken stages.
- No BIOS files.
