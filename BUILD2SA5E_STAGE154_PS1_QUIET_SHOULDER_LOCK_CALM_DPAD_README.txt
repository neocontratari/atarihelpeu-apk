AtariHelp EMU10 - BUILD2SA5E / STAGE154 PS1 overlay

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

Stage154 changes:
- Removed user-facing toast messages from L1/L2/R1/R2 long-press lock and unlock.
- Shoulder lock still works visually by the pressed/locked button style.
- D-pad is less sensitive near the center.
- Horizontal LEFT/RIGHT is now more deliberate during UP/DOWN walking.
- This should reduce accidental sideways movement while walking or aiming.

Current shoulder behavior:
- Short L1/L2/R1/R2 press = normal hold while finger is down.
- Long L1/L2/R1/R2 press = button stays held after finger release.
- Tap locked shoulder button again = unlock/release.

Expected test points:
- Medal of Honor: hold R2 longer to lock aim, tap R2 again to release, with no R2 toast message.
- Walking forward/back should tolerate small left/right finger drift without chaotic direction changes.
- Intentional diagonals still require a clearer push toward the side.
