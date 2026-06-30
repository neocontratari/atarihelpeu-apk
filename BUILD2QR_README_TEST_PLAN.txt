BUILD2QR TEST PLAN - Sega C++ only / no-autoboot / clean-sync audio

Aplikuj ZIP overlay pres BUILD2QQ.

TEST 1 - start Sega bez Javy:
1. Spust aplikaci.
2. Klikni SEGA MEGA DRIVE.
3. Spravny stav: nesmi se automaticky rozjet zadna stara Java hra ani stara native ROM.
4. Monitor muze byt cerny / ready text, ale nesmi hrat zadny zvuk.
5. Log ma obsahovat:
   - BUILD2QR_SEGA_NATIVE_CPP_ONLY_NO_AUTOBOOT_CLEAN_AUDIO_STAGE108
   - BUILD2QR LEGACY_JAVA_HARDBOOT_PURGED key=rom
   - BUILD2QR NATIVE_LAST_ROM_PURGED_NO_AUTOBOOT key=nativeRom
   - BUILD2QR CPP_ONLY_START_NO_AUTOBOOT armed waiting for fresh cartridge

TEST 2 - cisty Sonic C++:
1. Klikni SBIRKA/CARTRIDGE.
2. Vyber Sonic The Hedgehog (USA, Europe).gen.
3. Hraj 60 sekund: START, beh, skok, prstynky.
4. Sleduj sync a chrceni.
5. Log ma obsahovat:
   - BUILD2QR ROM SELECTED ... nativeCppOnly=YES
   - BUILD2QR NATIVE_ROM_TO_CPP
   - NATIVE_AUDIO_STREAM_START_QR_CLEAN_SYNC
   - audio_mode=FM_PSG_CLEAN_SYNC_QR
   - NATIVE_VIEW_REDRAW_BURST afterRomLoad

TEST 3 - navrat z Atari/130XE:
1. Ze Sonic C++ odejdi na Atari 130XE nebo VBXE.
2. Vrat se na Sega.
3. Spravny stav: zadna cerna obrazovka se zvukem. Nesmí hrat stara hudba.
4. Pro novy test znovu vyber Sonic pres SBIRKA/CARTRIDGE.

Co poslat zpet:
- Screenshot Sega po navratu z Atari/130XE.
- LOG po Sonic testu.
- Kratke hodnoceni: zvuk horsi/stejny/lepsi nez QO/QP/QQ, sync odhadem v sekundach.
