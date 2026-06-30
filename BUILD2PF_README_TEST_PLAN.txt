BUILD2PF_SEGA_FAST_INPUT_SAFE_AUDIO_ROLLBACK_STAGE70

DUVOD:
- BUILD2PE je STOP / mrtva vetev. Rozbil zvuk a core start.
- Log ukazal Web Audio playback buffer length mismatch 4096 vs expected 2048.
- Proto se vraci bezpecny audio zaklad z BUILD2PA a odstranuje se hard gate + ScriptProcessor patch.

ZMENY:
1) Zadne hard audio gate.
2) Zadne patchovani ScriptProcessor bufferu. ClownMDEmu nechava svuj ocekavany buffer 2048.
3) Zachovan mobilni multitouch joystick z BUILD2PA.
4) Landscape HUD safe bez ukrojenych zivotu a bez hornich duchu z PD.
5) Fast input quiet: v landscape se neloguji kazde tlacitko a wrapper neposila gamepadconnected pri kazdem inputu.
   Cil je snizit zatez postMessage/logovani, ktera muze na mobilu prispivat k zasekum zvuku.

TEST:
- Nox: hra musi znovu nabehnout a zvuk se musi vratit.
- Mobil: hra musi nabehnout, joystick jako 2PA, zadne horni duchy, zivoty/HUD videt.
- Zvuk: zkontrolovat, jestli fast input quiet snizil sekani/ozvenu.

LOG MARKERY:
- BUILD2PF_SEGA_FAST_INPUT_SAFE_AUDIO_ROLLBACK_STAGE70
- AUDIO_TUNE installed ... noHardGate=YES noScriptProcessorPatch=YES
- VIRTUAL_GAMEPAD_READY ... fastInputQuiet=YES
- INPUT_FAST_SUMMARY
- REGION_SAFE_ROLLBACK_2PA_PF
