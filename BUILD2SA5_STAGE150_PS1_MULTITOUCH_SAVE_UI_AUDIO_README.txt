BUILD2SA5_STAGE150_PS1_MULTITOUCH_SAVE_UI_AUDIO

PS1-only cumulative overlay.

Contains:
- app/src/main/assets/emu_ps1/index.html
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java
- app/src/main/cpp/nap_ps1_native.cpp

Fixes and changes:
- Real multi-touch input: D-pad and action buttons keep separate touch identifiers.
- Walking + shooting / crouch / sneak combinations can be held at the same time.
- Landscape UI hides BACK, CD ISO, and SETTINGS.
- Landscape UI shows LOAD, SAVE, and STATE.
- LOAD boots a PS1 game through the native picker.
- SAVE writes a libretro savestate slot for the current game.
- STATE loads that savestate slot.
- PS1 preview image quality raised from JPEG 72 to JPEG 88.
- Preview polling raised from 10 fps to about 15 fps.
- PS1 audio buffer/prefill increased for longer-session stability.

Notes:
- Save states are not the same as an in-game PlayStation memory card, but they provide playable position saves now.
- The PS1 core already receives a save directory for memory-card files if the libretro core writes them.
- No Sega emulator files.
- No Atari 130XE emulator files.
- No Gradle/build/install-signing changes.
