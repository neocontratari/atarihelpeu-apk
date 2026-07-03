BUILD2SA4B_STAGE149_PS1_UI_POLISH

PS1-only cumulative overlay.

Contains:
- app/src/main/assets/emu_ps1/index.html
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java
- app/src/main/cpp/nap_ps1_native.cpp

Purpose:
- Keeps the working PS1 audio/input changes from STAGE148.
- Replaces the rough landscape debug controls with a subtle playable overlay:
  large invisible hitboxes, small visual D-pad, PS action symbols, compact top buttons.
- D-pad math follows the visible D-pad, and action buttons follow the nearest visible PS symbol.
- Keeps widescreen PS1 preview in landscape.

Not included:
- No Sega emulator files.
- No Atari 130XE emulator files.
- No Gradle/build/install-signing changes.
