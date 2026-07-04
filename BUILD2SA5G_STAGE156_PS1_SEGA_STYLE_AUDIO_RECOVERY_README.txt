AtariHelp EMU10 - BUILD2SA5G / STAGE156 PS1 overlay

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

Why Stage156 exists:
- Stage155 was too aggressive for NfS and could make audio feel more stuck.
- The log showed audioDropped=0 and audioResyncs=0, so the hard FIFO governor was not the real active fix.
- The log also showed tiny FIFO fragments such as 1 or 5 frames; those should not be pushed to AudioTrack as separate crackly chunks.

Stage156 audio changes:
- PS1 AudioTrack now follows the working Sega approach more closely.
- Uses an S8/no-starve reservoir instead of the smaller Stage155 buffer.
- Does not write artificial silence when native audio is momentarily empty.
- On a low/empty pull, waits briefly and retries several times like Sega.
- Native ps1PullAudio now waits for almost a complete PS1 audio block instead of draining tiny 1-5 frame fragments.
- Native FIFO runaway guard remains, but is relaxed to about 400 ms before resync.

Stage156 graphics preview change:
- PS1 JPEG preview quality raised from 88 to 95.
- Preview poll changed from about 15 fps to about 20 fps.
- This is still the current JPEG preview path, not the future full TextureView renderer.

Carried forward:
- Larger calmer D-pad with movable center.
- Quiet L1/L2/R1/R2 long-press lock.
- Wide-screen Load/Save/State buttons remain hidden.

Expected test points:
- NfS audio should be less stuck/crackly than Stage155.
- If audio is still bad, export PS1 log with BUILD2SA5G lines.
- Watch PS1_AUDIO_WRITE underrunLoops and Android underruns.
- Watch PS1_RUN audioFifoFrames/audioDropped/audioResyncs.
