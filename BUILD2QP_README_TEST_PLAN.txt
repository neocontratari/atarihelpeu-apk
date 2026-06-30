AtariHelp.eu EMU-10 BUILD2QP_SEGA_NATIVE_CPP_REENTER_CLEAN_AUDIO_STAGE106

TEST PLAN - PRESNE:
1) Aplikuj ZIP overlay pres BUILD2QO.
2) Spust appku.
3) Hlavni menu -> SEGA MEGA DRIVE.
4) C++ CORE.
5) Vyber Sonic The Hedgehog (USA, Europe).gen pres SBIRKA/CARTRIDGE.
6) START, hrat 60 sekund: skok, prstynky, beh.
7) Bez resetu prejit na Atari 130XE nebo VBXE.
8) Vratit se na SEGA MEGA DRIVE.
9) Zmacknout C++ CORE znovu.

SPRAVNE:
- Sonic obraz nesmi zustat pres Atari/VBXE/Intro.
- Po navratu do Sega nesmi byt cerna obrazovka se zvukem.
- Pokud je posledni ROM v pameti, ma se znovu nahrat do C++ a ukazat obraz.
- Pokud ROM v pameti neni, musi to napsat vyber ROM, ne pustit cernou native vrstvu.
- Zvuk ma byt cistsi nez QO; sync by mel zustat blizko QO: Sonic cca 0.2 s nebo lepe, Aladdin cca 0.1 s.

LOG MARKERY:
- BUILD2QP_SEGA_NATIVE_CPP_REENTER_CLEAN_AUDIO_STAGE106
- NATIVE_REENTER_RELOAD_LAST_ROM
- NATIVE_ARMED_WAITING_FOR_ROM
- NATIVE_AUDIO_STREAM_START_QP_CLEAN
- NATIVE_AUDIO_STREAM_STOP_REQUEST_QP_CLEAN hardTrackStop=true
- audio_mode=FM_PSG_CLEAN_QP
- NATIVE_SHUTDOWN_OK_QP

CO POSLAT ZPET:
- Sonic: sync odhad, cistota zvuku, jestli chrci.
- Aladdin: sync odhad, cistota zvuku.
- Re-enter test: jestli po Atari/VBXE -> Sega -> C++ je obraz nebo cerna obrazovka.
- LOG po testu.

KODY JSOU ZMENENE.
