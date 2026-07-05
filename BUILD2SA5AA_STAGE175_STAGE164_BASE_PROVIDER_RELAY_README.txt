BUILD2SA5AA / STAGE175 - Stage164 base + provider relay only

Base:
- Built from BUILD2SA5P / STAGE164, because this was the last known build where local 130XE XEX games from mobile loaded correctly.

What changed:
- Added provider relay handling for atarihelp.eu / vedos.cz.
- WebView requests for those domains are intercepted and fetched through:
  1. https://proxy.cors.sh/
  2. https://api.allorigins.win/raw?url=
  3. https://corsproxy.io/?url=
- Downloaded web games from blocked provider URLs also use the relay path.
- Provider URLs opened through app buttons stay inside the app instead of jumping to the external browser.

What did NOT change:
- No Stage171/172/173/174 Atari injection changes.
- No XEX MOBIL bridge override.
- No 130XE asset/core edits.
- No Sega asset edits.
- No PS1 asset/core edits beyond the Stage164 base.
- No BIOS files included.

Log markers:
- BUILD2SA5AA PROVIDER_RELAY_REQUEST
- BUILD2SA5AA ATARIHELP_RELAY_LOAD
- BUILD2SA5AA PROVIDER_RELAY_OK / PROVIDER_RELAY_FAIL
- BUILD2SA5AA WEB_GAME_DOWNLOADED
