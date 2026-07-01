BUILD2RB_SEGA_NATIVE_CPP_ONLY_MULTIROM_AUDIO_VIDEO_FIX_STAGE118

CIL TESTU:
- Nevracet se zpet v opravach RA: Nox ma zustat OK, Samurai ma zustat OK, navrat z 130XE ma zustat OK.
- Opravit hlavni S8 problem po vice ROM: po kazdem dalsim nahrani hry se nesmi periodicky zhorsovat zvuk ani video.
- Opravit landscape orez: Sonic musi ukazovat spodní HUD/zivoty, A/B/C maji byt niz.

TEST 1 - NOX KONTROLA:
1. Sega -> SBIRKA -> Sonic.
2. Hrat 20 s.
3. Atari 130XE -> zpet Sega -> SBIRKA -> Aladdin/Sonic.
Ocekavani:
- Bez cerne obrazovky po navratu.
- Bez maleho videa vlevo nahore.
- Samurai Europe porad projde regionem.

TEST 2 - S8 MULTI-ROM:
1. S8 portrait: Sonic 30 s.
2. SBIRKA -> Aladdin 30 s.
3. SBIRKA -> Sonic znovu 30 s.
4. Volitelne Samurai.
Ocekavani:
- Po kazde dalsi ROM se video ani zvuk nesmi zpomalovat periodickou radou.
- V logu hledej NATIVE_AUDIO_STREAM_START_RB_QT_AUDIO_KEEP gen=... a stare geny se nesmi vratit.

TEST 3 - S8 LANDSCAPE:
1. Sonic -> otocit na sirku.
2. Hrat 20 s.
Ocekavani:
- Video bezi plynule.
- Spodni HUD/zivoty jsou viditelne.
- A/B/C jsou nize nez v RA.
- Ovládani je pruhledne a nerusi.

LOG MARKERY:
- BUILD2RB_SEGA_NATIVE_CPP_ONLY_MULTIROM_AUDIO_VIDEO_FIX_STAGE118
- FRESH_ROM_GENERATION_RB
- NATIVE_AUDIO_STREAM_START_RB_QT_AUDIO_KEEP gen=
- NATIVE_AUDIO_THREAD_STILL_ALIVE_RB invalidatedGen= (jen pokud stare vlakno nestihlo skoncit, ale nesmi zpusobit zpomaleni)
- NATIVE_TEXTURE_FRAME_RB ... gameDst=
