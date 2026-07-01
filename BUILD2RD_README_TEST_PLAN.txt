AtariHelp.eu EMU-10 BUILD2RD
SEGA_NATIVE_CPP_ONLY_MULTIROM_PERFORMANCE_LAYOUT_STAGE120

Test plan:

1) GitHub workflow
- Aplikovat overlay pres BUILD2RC.
- Commit + push.
- Ocekavani: workflow projde a vytvori jednu APK.

2) NOX kontrola
- Sega -> SBIRKA -> Sonic.
- Sonic -> Atari 130XE -> zpet Sega -> Sonic / Aladdin.
- Ocekavani: bez cerne obrazovky, bez zvuku bez obrazu.

3) S8 prvni Sonic
- Sega -> SBIRKA -> Sonic.
- Ocekavani: sync zustava dobry, video stream plynule jako v RA/RC nebo lepsi.

4) S8 landscape
- Pri bezici hre otocit na sirku.
- Ocekavani: obraz neni s bocnimi pruhy, A/B/C jsou niz u dolniho praveho okraje, DPAD/A/B/C maji vetsi touch citlivost.

5) S8 multi-ROM
- Sonic 30s -> Aladdin/Asterix 30s -> Sonic 30s -> Samurai 30s.
- Ocekavani: po kazde dalsi ROM se video ani zvuk nesmi periodicky zhorsovat.

Log markery:
- BUILD2RD_SEGA_NATIVE_CPP_ONLY_MULTIROM_PERFORMANCE_LAYOUT_STAGE120
- NATIVE_VIEW_REUSE_RD parent=rootFrame noRecreate=YES
- MOBILE_NATIVE_RECT_RECOVER_ONLY reason=orientationchange/resize/visibility-visible noEnableInPlace=YES
- NATIVE_AUDIO_STREAM_STOP_REQUEST_RD_QT_AUDIO_KEEP hardReleaseTrack=true
- NATIVE_TEXTURE_FRAME_RD ... landscapeFull=true
