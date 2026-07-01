BUILD2RA_SEGA_NATIVE_CPP_ONLY_CLEAN_AUDIT_RENDER_STAGE117

CIL:
Ne dalsi lepeni jedne ROM. BUILD2RA opravuje dve systemove chyby QZ:
- mala Sonic obrazovka vlevo nahore v Noxu,
- mobilni S8 video bezi extrémne pomalu, protoze render cekal na core/lock.

TEST:
1) Nox portrait:
   Sega -> SBIRKA -> Sonic.
   Spravne: zadny maly obraz vlevo nahore; obraz jen v monitoru.

2) Nox lifecycle:
   Sonic -> Atari 130XE -> zpet Sega -> Sonic/Aladdin.
   Spravne: bez cerne obrazovky se zvukem.

3) S8 portrait:
   Sega -> SBIRKA -> Sonic.
   Spravne: video musi byt vyrazne plynulejsi nez QZ. Nehodnotit jen zvuk; hlavni je obraz.

4) S8 landscape:
   Sonic -> otocit na sirku.
   Spravne: obraz pres celou sirku, pruhledny joystick, bez hornich textu a bez C++ CORE tlacitka.

5) Samurai Europe:
   Rychla kontrola, ze stale jede. Region se nelepi po hre; respektuje se ROM header.

LOG MARKERY:
- BUILD2RA_SEGA_NATIVE_CPP_ONLY_CLEAN_AUDIT_RENDER_STAGE117
- NATIVE_VIEW_REATTACH_RA hiddenUntilValidRect=YES
- NATIVE_RECT_FALLBACK_MONITOR
- NATIVE_TEXTURE_THREAD_START_RA
- NATIVE_TEXTURE_FRAME_RA
- REAL_CORE_RENDER_OK_WORKER_THREAD_RA
- displayFrameCounter=
- NO_PATCH_RESPECT_ROM_HEADER

POZNAMKA:
Zvuk je ponechany na QT/QP profilu. Tento build ma cilove resit video/render bordel a QZ top-left chybu.
