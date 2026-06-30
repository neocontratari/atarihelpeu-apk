BUILD2QO TEST PLAN - Rene / AtariHelp.eu

APLIKACE:
- Overlay nakopirovat pres BUILD2QN projekt.
- V Android Studio / GitHub Desktop: commit napriklad:
  BUILD2QO Sega C++ lifecycle + Sonic audio sync

TEST 1 - Sonic native C++ start:
1. Spust appku.
2. Intro -> SEGA MEGA DRIVE.
3. Zmackni C++ CORE.
4. Pres CARTRIDGE / SBIRKA vyber: Sonic The Hedgehog (USA, Europe).gen.
5. Pockej na obraz a hudbu, zmackni START, hraj 30-60 sekund.

Ocekavany vysledek:
- Sonic jede v Sega monitoru.
- Hudba nesmi mit pulsekundove zpozdeni.
- Zvuk by mel byt cistsi nez BUILD2QN: mene chrceni, mene trhani.
- Jump/ring PSG efekty maji zustat slyset.

Log markery:
- BUILD2QO_SEGA_NATIVE_CPP_LIFECYCLE_AUDIO_SYNC_STAGE105
- NATIVE_AUDIO_STREAM_START_QO_SYNC
- NATIVE_AUDIO_PREFILL_QO
- NATIVE_AUDIO_PULL_QO_SYNC
- realCoreStatus obsahuje audio_mode=FM_PSG_SYNC_QO
- realCoreStatus/status obsahuje frameClockNs=16666667 pro NTSC Sonic

TEST 2 - kriticka chyba okno vsude:
1. Nech Sonic bezet v C++.
2. Jdi ze Sega pres MENU na uvodni obrazovku.
3. Pak otevri Atari 130XE / VBXE / PS1 / Player.

Ocekavany vysledek:
- Sonic obraz NESMI zustat pres Intro/VBXE/Atari/PS1.
- Sonic hudba/zvuk se musi zastavit.
- Neni potreba resetovat celou appku.

Log markery:
- NATIVE_LIFECYCLE_STOP_BEGIN reason=...
- NATIVE_SHUTDOWN_OK_QO worker=STOPPED audio=CLEARED input=RELEASED
- NATIVE_OFF_LIFECYCLE_STOP

Co poslat zpet pri neuspechu:
- Screenshot kde je Sonic okno spatne videt.
- LOG z diskety ULOZENE.
- Napis zarizeni: Nox / S8 / A12 a Android verzi.
- Pro zvuk: napis jestli je problem chrceni, prodleva, nebo oboji.
