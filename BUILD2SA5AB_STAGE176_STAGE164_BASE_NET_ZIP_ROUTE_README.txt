AtariHelp EMU10 BUILD2SA5AB / Stage176

Base:
- Stage164 behavior kept for local 130XE XEX/ATR picker path.
- Stage175 provider relay kept for AtariHelp/WEDOS blocked network path.

Fixes in this overlay:
- Failed NET HRY download no longer opens 130XE as a fallback.
- Sega collection ZIP downloads are routed through a Sega-specific path.
- Sega ZIP content is inspected and .gen/.md/.smd/.sms/.68k/.sgd payload is injected into the Sega emulator.
- Atari web ZIP content is inspected and .xex/.com/.exe payload is preferred, with .atr as fallback.
- WordPress image/figure clicks are bridged to AHNET.runGameUrl too.

Not changed:
- No Sega emulator core/assets edits.
- No 130XE emulator core/assets edits.
- No local mobile XEX picker rewrite from later broken stages.
- No BIOS files.
