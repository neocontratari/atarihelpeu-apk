BUILD2QW TEST PLAN

Install over BUILD2QV.

CIL:
- Nehybat se zvukem: zustava QT/QP clean profile z QV/QT.
- Opravit cerny obraz + zvuk po navratu z Atari 130XE.
- Opravit realny mobil landscape: video pres celou obrazovku, joystick a A/B/C jsou pruhledne nad obrazem.
- Zlepsit rychlost obrazu na S8/A12: Java View uz nevyzaduje obri full-screen render z C++, bere 320x224 a skaluje.

NOX TEST:
1. Spust appku.
2. SEGA MEGA DRIVE -> SBIRKA -> Sonic.
3. Over zvuk: musi byt stejny smer jako QV/QT, ne novy experiment.
4. Jdi Atari 130XE -> vrat se Sega -> SBIRKA -> Aladdin/Sonic.
5. Ocekavani: nesmi zustat cerna obrazovka se zvukem.

S8 / A12 TEST:
1. Spust appku na mobilu.
2. SEGA MEGA DRIVE -> SBIRKA -> Sonic.
3. Otoc do landscape.
4. Ocekavani: obraz neni jen horni pruh; video je pres celou obrazovku.
5. Joystick a A/B/C jsou pruhledne nad obrazem a nesmi vyrazne rusit.
6. Obraz nesmi byt tak kouskovany jako BUILD2QV.

LOG MARKERY:
- BUILD2QW_SEGA_NATIVE_CPP_ONLY_MOBILE_VIDEO_UI_THREAD_STAGE113
- NATIVE_VIEW_REMOVE_UI_OK_QW
- NATIVE_Z_ORDER_QW landscapeWebControlsOverNative=YES
- NATIVE_RECT_DPR ... landscapeFullVideo=YES
- VISIBILITY_HIDDEN_IGNORED_ROM_PICKER
- EU_COMPAT_SKIP_WORLD_SONIC
- NATIVE_AUDIO_STREAM_START_QW_QT_AUDIO_KEEP
- NATIVE_RENDER_WATCHDOG_QW
