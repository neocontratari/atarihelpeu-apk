BUILD2QS TEST PLAN
==================

BUILD:
BUILD2QS_SEGA_NATIVE_CPP_ONLY_QP_AUDIO_RENDER_WATCHDOG_STAGE109

KODY JSOU ZMENENE.

TEST 1 - Sonic hlavni test:
1. Aplikuj overlay pres BUILD2QR.
2. Spust aplikaci.
3. Hlavni menu -> SEGA MEGA DRIVE.
4. Spravne: nesmi se sama spustit Java hra ani stary zvuk.
5. SBIRKA/CARTRIDGE -> vyber Sonic The Hedgehog (USA, Europe).gen.
6. Hraj 60 sekund: START, beh, skok, prstynky.

OCEKAVANI:
- Zvuk ma byt bliz QP: cistsi nez QR/QQ.
- Sync muze byt lehce pomalejsi nez nejagresivnejsi QQ, ale nesmi se vratit k 0.5 s.
- Zadny Java wrapper / zadny Java sum.

TEST 2 - odchod a navrat:
1. Ze Sonica odejdi do Atari 130XE / VBXE / MENU.
2. Vrat se na Sega.
3. Spravne: zadny Sonic obraz pres jinou stranku, zadny Sonic zvuk mimo Sega.
4. Po navratu Sega ceka na novy vyber ROM.

TEST 3 - Aladdin EU obraz:
1. Na Sega vyber Aladdin (Europe).gen.
2. Pockej 3 sekundy.
3. Sleduj, jestli se objevi obraz.

OCEKAVANI:
- Pokud prvni init neda frame, watchdog jednou reloadne native core.
- Nesmí zustat dlouhodobe stav "zvuk jede, obraz cerny".
- Kdyz frame neprijde ani po watchdogu, audio se vypne a log to prizna.

LOG MARKERY:
- BUILD2QS_SEGA_NATIVE_CPP_ONLY_QP_AUDIO_RENDER_WATCHDOG_STAGE109
- NATIVE_AUDIO_STREAM_START_QS_QP_CLEAN
- audio_mode=FM_PSG_QP_CLEAN_VIDEO_WATCHDOG_QS
- NATIVE_EU_COMPAT_FOR_CPP
- frameReady=YES
- frameCounter=
- NATIVE_RENDER_WATCHDOG_QS
- NATIVE_RENDER_WATCHDOG_RELOAD_QS
- NATIVE_RENDER_NO_FRAME_AUDIO_STOPPED_QS
- NATIVE_SHUTDOWN_OK_QS_CPP_ONLY

CO POSLAT ZPET:
- Sonic: zvuk cistsi/stejny/horsi proti QP a QR.
- Sonic: odhad prodlevy.
- Aladdin: obraz naskocil / watchdog reload / zustal bez obrazu.
- Log ze Sonica a log z Aladdina.
