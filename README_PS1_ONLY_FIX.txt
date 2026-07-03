BUILD2SA3 STAGE146 FIX1 - PS1 ONLY

Problem:
GitHub Actions failed during compileDebugJavaWithJavac:
MainActivity.java:1398: cannot find symbol variable AudioManager.

Scope:
Only PS1 audio compile fix.
No emu_sega asset change.
No Atari 130XE asset/core/routing change.
No web ZIP routing change.

Code change:
In startPs1Audio(), the AudioTrack constructor now uses the fully qualified Android class:
android.media.AudioManager.STREAM_MUSIC

This matches the already working older AudioTrack fallback style in the same file and avoids adding any wider import or behavior change.

Changed file:
app/src/main/java/eu/atarihelp/emu10/MainActivity.java

Exact changed line:
- at = new AudioTrack(AudioManager.STREAM_MUSIC, ...)
+ at = new AudioTrack(android.media.AudioManager.STREAM_MUSIC, ...)

Expected result:
GitHub Java compile should pass this previous error and continue to the next build step.
