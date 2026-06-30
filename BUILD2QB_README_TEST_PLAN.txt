AtariHelp.eu EMU-10 BUILD2QB_SEGA_NATIVE_CPP_OFFLINE_VENDOR_PCM_HEADER_FIX_STAGE92

KODY JSOU ZMENENE: ANO

2QA / 2PX / 2PY / 2PZ ZAHODIT.

CO TENTO OVERLAY OPRAVUJE:
- BUILD2QA spadl v native C/C++ buildu.
- Lokalni kontrola vendor zdrojaku ukazala konkretni problem v ClownMDEmu-core: source/pcm.h deklaroval PCM_ReadRegister(... cc_u8f reg), ale source/pcm.c definuje PCM_ReadRegister(... cc_u16f reg).
- To je C compile error: conflicting types for PCM_ReadRegister.
- BUILD2QB opravuje jen tuto vendor header signaturu a nechava offline/vendor strategii beze zmen.
- Zadny GitHub fetch, zadne submodule tahani, zadne disassembler tooly, zadne microcode generator targety, zadne test targety.

CIL:
- Nejdriv projit GitHub Actions build.
- Pokud build projde, testovat normalni Sega obrazovku -> C++ CORE -> vyber Aladdin/Sonic -> ULOZENE log.

TEST PLAN:
1. Nasad BUILD2QB overlay pres GitHub Desktop.
2. Commit/push.
3. Pockej na GitHub Actions.
4. Pokud build spadne, posli cervene radky nad 'BUILD FAILED', hlavne prvni error z compileru/ninja/CMake.
5. Pokud build projde: SEGA -> C++ CORE -> vyber ROM -> screenshot + ULOZENE log.

LOG MARKERY:
- BUILD2QB_SEGA_NATIVE_CPP_OFFLINE_VENDOR_PCM_HEADER_FIX_STAGE92
- REAL_CORE_PRESENT=YES
- vendor_offline=local ZIP sources
- REAL_CORE_LOAD_OK nebo presny REAL_CORE error

POCTIVE:
Tohle jeste negarantuje hratelny Sonic/Aladdin. Je to oprava compile failu z 2QA, aby se realny core vubec dostal do APK.
