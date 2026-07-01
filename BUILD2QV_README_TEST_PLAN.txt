BUILD2QV TEST PLAN

Aplikuj pres BUILD2QU nebo posledni pracovni Sega build.

Test 1 - Sonic zvuk:
1. Spust appku.
2. SEGA MEGA DRIVE.
3. SBIRKA/CARTRIDGE.
4. Vyber Sonic The Hedgehog (USA, Europe).gen.
5. Hraj 45-60 s: skoky, prstynky, beh.
Ocekavani: zvuk jako BUILD2QT/QP clean, uz ho ted nemenime; popis jen prodlevu.

Test 2 - navrat z Atari 130XE:
1. Ze Sonica odejdi do Atari 130XE.
2. Vrat se do Sega.
3. SBIRKA/CARTRIDGE.
4. Vyber Aladdin nebo Sonic.
Ocekavani: nesmi zustat stav "cerna obrazovka + zvuk".
Kdyz neni obraz, zvuk se nesmi spustit nebo se musi zastavit a log to prizna.

Log markery:
- BUILD2QV_SEGA_NATIVE_CPP_ONLY_QT_AUDIO_BLACKSCREEN_GUARD_STAGE112
- NATIVE_VIEW_RECREATE_QV
- FRESH_ROM_HARD_STOP_BEFORE_LOAD_QV
- NATIVE_AUDIO_WAIT_FRAME_VIEW_QV
- NATIVE_AUDIO_START_AFTER_FRAME_VIEW_QV
- NATIVE_RENDER_WATCHDOG_QV
- NATIVE_RENDER_NO_VISIBLE_FRAME_AUDIO_STOPPED_QV
- NATIVE_AUDIO_STREAM_START_QV_QT_AUDIO_KEEP
