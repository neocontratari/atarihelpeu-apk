AtariHelp EMU10 - BUILD2SA5F / STAGE155 PS1 overlay

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

Stage155 audio changes:
- PS1 native audio FIFO is no longer allowed to grow into a long stale queue.
- FIFO now targets about 50 ms and hard-resyncs if it grows above about 133 ms.
- PS1 status now reports audioResyncs next to audioDropped.
- Java AudioTrack uses a smaller requested stream buffer.
- Java prefill is reduced from a large delay to a small jitter cushion.
- If the PS1 core has a tiny audio gap, Java writes a short silence cushion instead of letting Android underrun harshly.
- Audio logs now include BUILD2SA5F, actual AudioTrack frame count, silenceFrames, and Android underrun count when available.

Carried from Stage154:
- Larger calmer D-pad with movable center.
- Quiet L1/L2/R1/R2 long-press lock.
- Wide-screen Load/Save/State buttons remain hidden.

Expected test points:
- Run a game long enough for the old crackle to appear.
- If crackle is improved, this was queue drift or Android underrun.
- If crackle remains, export the PS1 log and check:
  audioResyncs > 0 means native FIFO was overflowing and old audio was dropped.
  audioDropped > 0 means the governor actively kept latency short.
  silenceFrames rising fast means the interpreter/core is starving the audio output.
  underruns rising means Android AudioTrack still runs dry.
