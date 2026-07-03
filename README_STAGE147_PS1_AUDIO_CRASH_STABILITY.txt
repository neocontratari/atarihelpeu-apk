BUILD2SA3B / STAGE147 - PS1 AUDIO CRASH STABILITY

Use this over the working STAGE146 PS1 audio build.
Do not use the previous FIX2 mobile-install overlay for this test.

Confirmed from user test:
- Medal of Honor: PS1 image and sound work.
- Crash Bandicoot: image works, audio stutters/cuts.

What the log showed:
- Both tested PS1 boots report fps=60.00 and sampleRate=44100.
- audioFifoFrames often drops to 0, so the Java AudioTrack path has no reserve.
- This looks more like PS1 audio buffering/lifecycle than a PAL/EU/USA/Japan region issue.

Code changes:
1. MainActivity.java
   - Stops the old PS1 AudioTrack before a new PS1 boot.
   - Tracks the current PS1 AudioTrack and audio thread.
   - Uses a larger AudioTrack reservoir.
   - Prefills about 4096 frames before playback starts.
   - Uses blocking writes on API 23+.
   - Keeps the existing compile fix: android.media.AudioManager.STREAM_MUSIC.

2. nap_ps1_native.cpp
   - Clears PS1 native audio FIFO on boot.
   - Clears PS1 native audio FIFO on stop.

Not changed:
- No app/build.gradle change.
- No install metadata change.
- No emu_sega asset change.
- No Atari 130XE asset/core/routing change.

Expected test:
1. Build debug APK in GitHub Actions from this overlay.
2. Install on S8/mobile.
3. Boot Medal of Honor once to confirm no regression.
4. Boot Crash Bandicoot directly after app restart.
5. Then boot Medal of Honor -> Crash without app restart.
6. In log look for BUILD2SA3B PS1_AUDIO_START and prefillFrames.
