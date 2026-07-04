AtariHelp EMU10 - BUILD2SA5H / STAGE157 PS1 overlay

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

Why Stage157 exists:
- Stage156 still left a repeated 468/469-frame audio remainder in the PS1 FIFO.
- That happened because Java asked for 735 frames while the core naturally left smaller chunks behind.
- Waiting for a near-full 735-frame block made the audio pulse and stutter.

Stage157 audio changes:
- PS1 Java AudioTrack now pulls 384-frame chunks, closer to the proven Sega audio path.
- Native ps1PullAudio no longer waits for almost 735 frames.
- Native pull only refuses tiny crumbs under 128 frames.
- No artificial silence is written.
- S8/no-starve AudioTrack reservoir remains enabled.

Stage157 BIOS boot changes:
- PCSX-ReARMed core option pcsx_rearmed_bios is set to auto.
- PCSX-ReARMed core option pcsx_rearmed_show_bios_bootlogo is set to enabled.
- Region remains auto.
- BIOS upload now also writes a canonical scph5501.bin alias when a 512 KB BIOS arrives with a nonstandard name.

Important:
- Sony boot logo and music need a real BIOS file in the PS1 system directory.
- If the core falls back to HLE BIOS, the logo/music may still not appear.
- If this still does not show the Sony boot, re-upload a real SCPH BIOS through the PS1 BIOS flow.

Carried forward:
- JPEG95 PS1 preview.
- Larger calmer D-pad with movable center.
- Quiet L1/L2/R1/R2 long-press lock.
- Wide-screen Load/Save/State buttons remain hidden.

Expected test points:
- NfS audio should stop pulsing on the 468/469-frame FIFO remainder.
- Check logs for BUILD2SA5H PS1_AUDIO_START chunk=384.
- Check PS1_RUN audioFifoFrames; it should not sit forever around 468/469.
- Check whether Sony boot logo/music appears on a fresh game boot after real BIOS is saved.
