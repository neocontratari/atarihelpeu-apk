BUILD2QU TEST PLAN

Testuj v Noxu, mobil S8/A12 zatim NE, dokud Nox nebude 100 %.

1) Aplikuj BUILD2QU pres posledni BUILD2QT.
2) Spust appku.
3) Sega Mega Drive.
   Spravne: zadna Java hra sama nenabehne, zadny stary zvuk.
4) SBIRKA/CARTRIDGE -> Sonic The Hedgehog (USA, Europe).gen.
5) Test 60 sekund:
   - START
   - skok B
   - prstynky
   - rychly beh doprava

Ocekavani Sonic:
- tlacitka funguji jako v QT.
- zvuk nema byt horsi nez QT.
- zpozdeni musi byt mensi nez QT 0.8 s; cil je vratit se bliz QO/QP.
- v logu Sonic JUE NESMI byt zbytecne patchnuty na U; hledej EU_COMPAT_SKIP_WORLD.

6) Prejdi do Atari 130XE.
7) Vrat se do Sega.
8) Vyber Aladdin (Europe).gen pres SBIRKA/CARTRIDGE.

Ocekavani Aladdin:
- nesmi zustat jen zvuk bez obrazu.
- pokud obraz nenaskoci, audio se ma samo zastavit a log ma priznat NO_FRAME.
- zadna Java/WebView Sega cesta.

Log markery:
- BUILD2QU_SEGA_NATIVE_CPP_ONLY_FRESH_VIDEO_QO_LATENCY_STAGE111
- FRESH_ROM_HARD_STOP_BEFORE_LOAD
- VISIBILITY_HIDDEN_IGNORED_ROM_PICKER
- NATIVE_AUDIO_WAIT_FIRST_FRAME_QU
- NATIVE_AUDIO_START_AFTER_FIRST_FRAME_QU
- NATIVE_AUDIO_STREAM_START_QU_QO_LATENCY
- audio_mode=FM_PSG_QO_LATENCY_FRESH_VIDEO_QU
- EU_COMPAT_SKIP_WORLD pro Sonic JUE
- JS_RENDER_WATCHDOG
- NATIVE_RENDER_NO_FRAME_AUDIO_STOPPED_QU pouze pokud obraz opravdu nenaskoci

Pokud selze:
- posli aplikacni log a napis: Sonic zvuk lepsi/horsi nez QT, zpozdeni odhad, a jestli po Atari->Sega->Aladdin byl obraz nebo jen zvuk.
