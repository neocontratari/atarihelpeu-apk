BUILD2SA4 / STAGE148 - PS1 INPUT + WIDE SCREEN + AUDIO CLEANUP

Use this after the working STAGE147/SA3B line.
Do not use the discarded FIX2 mobile-install overlay.

Scope:
- PS1 only.
- No app/build.gradle change.
- No install metadata change.
- No Sega assets.
- No Atari 130XE assets/core/routing.

What changed:
1. Real PS1 input
   - Native PS1 input callback no longer returns 0 forever.
   - Java exposes AHPS1.ps1Input(button, down).
   - PS1 page touch zones now hold real RetroPad bits:
     D-pad: UP/DOWN/LEFT/RIGHT
     action cluster: TRIANGLE/CIRCLE/CROSS/SQUARE
     center buttons: SELECT/START

2. Playable screen
   - PS1 preview image now uses object-fit: fill instead of contain.
   - Landscape CSS stretches the monitor across the screen and keeps usable overlay controls.
   - START/SELECT hitboxes are larger in portrait.

3. Audio cleanup
   - Keeps STAGE147 hard-stop and FIFO reset behavior.
   - AudioTrack buffer is larger.
   - Prefill target is larger.
   - Pull chunk is aligned around 735 frames for 60 Hz / 44100 Hz.
   - PCM gain is reduced to 7/8 to reduce speaker crackle.

Test order:
1. Diablo title screen: press START. It should continue.
2. Menu/game: D-pad should move.
3. Action cluster should produce PS1 face-button input.
4. Rotate to landscape: screen should fill wide play area.
5. If audio still crackles, save PS1 log again; next step is native audio pacing/dynarec, not UI.
