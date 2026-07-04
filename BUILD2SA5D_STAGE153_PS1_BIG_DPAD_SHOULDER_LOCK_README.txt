AtariHelp EMU10 - BUILD2SA5D / STAGE153 PS1 overlay

Scope:
- PS1 only.
- Does not touch Sega emulator files.
- Does not touch Atari 130XE emulator files.
- Does not include Gradle, workflow, CMake, or project metadata changes.

Files in this overlay:
- app/src/main/assets/emu_ps1/index.html
- app/src/main/cpp/nap_ps1_native.cpp
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java

Stage153 changes:
- Larger landscape D-pad.
- D-pad now has a movable center knob, so the finger direction is visually visible.
- L1, L2, R1, and R2 support long-press lock.
- Short shoulder press behaves like a normal held PlayStation button.
- Long shoulder press keeps the button pressed after releasing the finger.
- Tapping a locked shoulder button again unlocks it.
- Intended Medal of Honor use: hold R2 longer to lock aiming, tap R2 again to release.
- Landscape Load/Save/State controls remain hidden; normal portrait controls remain the place for load/save/memory-card flow.

Carried from earlier PS1 stages:
- Multi-touch fixes for D-pad plus action buttons.
- L1/L2/R1/R2 wide-screen controls.
- PS1 audio buffer/gain stability work.
- Native quick-save/quick-load bridge support.
- Improved PS1 preview rendering.

Expected test points:
- In Medal of Honor: R2 long press should keep aim held until tapped again.
- In games needing crouch/strafe/fire combinations: D-pad and action/shoulder buttons should work together.
- In landscape: D-pad should feel larger and easier to aim with.
