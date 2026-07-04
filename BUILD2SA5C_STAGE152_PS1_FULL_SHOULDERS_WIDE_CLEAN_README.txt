BUILD2SA5C_STAGE152_PS1_FULL_SHOULDERS_WIDE_CLEAN

PS1-only cumulative overlay.

Contains:
- app/src/main/assets/emu_ps1/index.html
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java
- app/src/main/cpp/nap_ps1_native.cpp

Changes from STAGE151:
- Adds L1 and L2 touch buttons in landscape gameplay mode.
- Keeps R1 and R2 touch buttons.
- Wide gameplay overlay now has complete PS1 shoulder buttons: L1/L2/R1/R2.
- Landscape still hides LOAD, SAVE, STATE / MEMORY CARD, BACK, CD ISO, SETTINGS.
- Portrait mode keeps normal console/menu buttons for LOAD/SAVE/MEMORY CARD.

Kept from earlier stages:
- Multi-touch D-pad/action fix.
- PS1 savestate bridge.
- Higher-quality PS1 preview.
- Larger PS1 audio buffer/prefill.

Not included:
- No Sega emulator files.
- No Atari 130XE emulator files.
- No Gradle/build/install-signing changes.
