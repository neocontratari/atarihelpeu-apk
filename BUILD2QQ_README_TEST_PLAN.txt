BUILD2QQ TEST PLAN - Sega C++ only + lower latency audio

Apply over BUILD2QP.

Goal:
- Sega page must not boot old Java/WebView wrapper first.
- ROM must go directly to C++ native core.
- After leaving to Atari/VBXE/Menu and returning, C++ must reload last native ROM or ask for ROM, not show black native screen with sound.
- Sonic sync should move closer to BUILD2QO (~0.2 s) while keeping QP cleaner sound.

Test 1 - fresh start / Sonic:
1. Start app.
2. Open SEGA MEGA DRIVE.
3. EXPECTED: no old Java game should auto-run in monitor.
4. Press C++ CORE or CARTRIDGE/SBIRKA and choose Sonic The Hedgehog (USA, Europe).gen.
5. Play 60 seconds: START, run, jump/ring.
6. Check if audio is clean and delay is lower than QP 0.4 s.

Test 2 - return/re-enter:
1. While Sonic is running in C++, go to Atari 130XE or VBXE or MENU.
2. EXPECTED: Sonic picture and sound stop outside Sega page.
3. Return to SEGA MEGA DRIVE.
4. EXPECTED: no Java wrapper starts first.
5. Press C++ CORE if it does not autoload.
6. EXPECTED: Sonic reloads in C++; no black screen with only sound.

Test 3 - Aladdin:
1. Select Aladdin (Europe).gen.
2. Confirm it goes direct C++ only.
3. Check sync and sound cleanliness.

Log markers to find:
- BUILD2QQ_SEGA_NATIVE_CPP_ONLY_LOW_LATENCY_STAGE107
- BUILD2QQ LEGACY_JAVA_HARDBOOT_PURGED key=rom
- BUILD2QQ WRAPPER_DISABLED_CPP_ONLY (should appear only if old wrapper path was accidentally requested)
- BUILD2QQ ROM SELECTED ... nativeCppOnly=YES
- BUILD2QQ NATIVE_LAST_ROM_STORE OK
- BUILD2QQ NATIVE_ROM_TO_CPP
- NATIVE_AUDIO_STREAM_START_QQ_LOW_LATENCY
- audio_mode=FM_PSG_LOW_LATENCY_QQ
- NATIVE_SHUTDOWN_OK_QQ_CPP_ONLY

Return to next assistant:
- Screenshot if Java wrapper still appears first.
- Log from Sonic test.
- Say estimated delay: 0.1 / 0.2 / 0.3 / 0.4 s.
- Say whether sound is clean, slightly chraply, or badly chraply.
