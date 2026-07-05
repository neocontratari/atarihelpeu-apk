AtariHelp EMU10 BUILD2SA5AF / Stage180

Base:
- Stage164 local 130XE XEX/ATR picker behavior kept.
- Stage176/177 ZIP routing kept.
- Stage178 local XEX bridge restored as the main NET HRY path.

Fix in this overlay:
- AtariHelp/WEDOS are again treated as provider-blocked domains.
- WebView uses the existing protected relay/intercept path for those domains.
- NET HRY opens the local Atari XEX bridge, so game clicks go straight through AHNET.runGameUrl().
- ZIP downloads still target atarihelp.eu and are fetched through the protected fetch path.
- Atari web ZIP content is inspected and .xex/.com/.exe payload is sent to 130XE, with .atr as fallback.
- Added log markers BUILD2SA5AF WEBVIEW_PROTECTED_WEB, PROVIDER_RELAY_REQUEST, ATARI_NET_BRIDGE_OPEN, WEB_GAME_DOWNLOADED, ZIP_CONTAINS_ATARI.

Not changed:
- No Sega emulator core/assets edits.
- No 130XE emulator core/assets edits.
- No local mobile XEX picker rewrite from later broken stages.
- No BIOS files.
