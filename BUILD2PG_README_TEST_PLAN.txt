BUILD2PG TEST PLAN

1) NEJDRIV NOX
- Spust Sega, Sonic.
- Ocekavani: hra nabehne, zvuk se vrati, zadne PE ticho, zadny 4096/2048 error.

2) MOBIL ONLINE
- Spust Sonic nebo Aladdin.
- Ocekavani: nabeh musi byt stabilni.
- Joystick musi zustat jako BUILD2PA: pohyb + skok zaroven.

3) MOBIL ZVUK
- Porovnej proti BUILD2PA/PF.
- Sleduj: ozvena, sekani, chrceni.

4) EU ROM
- Zkus Aladdin Europe nebo Mega Games 2 Europe.
- V LOGu ma byt u EU-only ROM: AUTO REGION -> EUROPE / PAL / International (NO_FORCE_NTSC) a NO_PATCH_PAL_AUTO_ORIGINAL_COPY.

5) CO POSLAT ZPET
- Screenshot jen kdyz obraz/duchove spatne.
- Vzdy LOG pres ULOZENE disketu.

KLICOVE LOG MARKERY:
- BUILD2PG_SEGA_SAFE_REBASE_AUTO_REGION_AUDIO_STAGE71
- AUDIO_TUNE installed latencyHint=playback preferredSampleRate=48000 ... audioPrime=ON ... noHardGate=YES noScriptProcessorPatch=YES
- REGION_SAFE_REBASE_2PG
- AUTO REGION -> EUROPE / PAL / International (NO_FORCE_NTSC) pro EU-only ROM
- LANDSCAPE_MULTITOUCH_ENGINE_READY
