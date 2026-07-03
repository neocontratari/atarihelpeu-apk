BUILD2SA1_FIX_ANDROID_MK_ALIGN_STAGE143B - OPRAVA CERVENEHO WORKFLOW

CO SE STALO (poctive):
SA1 seznam zdrojaku jsem vzal z LINUX buildu jadra. Ten obsahoval podporu
FYZICKE CD-ROM mechaniky (HAVE_CDROM + libretro-cdrom.c + vfs cdrom) - to je
linux-only kod, ktery na Androidu neexistuje. NDK build proto spadl.
Muj Linux test tohle nechytil, protoze na Linuxu to legalne kompiluje.

OPRAVA (3 male soubory, prepisi ty z SA1 commitu):
1) app/src/main/cpp/CMakeLists.txt
   - PS1 seznam zdrojaku ted PRESNE kopiruje jni/Android.mk primo z repa
     PCSX-ReARMed (jejich udrzovana Android konfigurace): 77 zdrojaku,
     zadne HAVE_CDROM, zadny linux-only kod, + ANDROID/REARMED defines.
   - NOVY KILL-SWITCH: option(NAP_ENABLE_PS1 ... ON).
     KDYBY PS1 JESTE ZLOBIL: zmen ON -> OFF na tom radku a workflow je
     okamzite zeleny bez PS1 (Sega nedotcena). Pak mi posli text chyby
     z cerveneho kroku v Actions a opravim cilene.
2+3) vendor/pcsx_rearmed/include/revision.h + frontend/revision.h
   - jadro je generuje gitem pri jejich buildu; u nas jsou pevne (r26-nap-sa1).

OVERENO U ME:
- napps1core s novym seznamem: BUILD OK + realne volani jadra:
  PS1_CORE_ALIVE api=1 name=PCSX-ReARMed version=r26
- Cely projekt: Sega link na mem Linuxu pada JEN kvuli chybejicimu -fPIC
  v mem testovacim prostredi (NDK ho dava automaticky; RY/RZ buildy #848-851
  byly v Actions zelene se stejnou Sega castou).

TEST: commit techto 3 souboru -> Actions MUSI byt zeleny. Pak stejny test
plan jako SA1 (Sega hraje stejne; PS1 stranka -> LOG -> PS1_CORE_PROBE
PS1_CORE_COMPILED name=PCSX-ReARMed version=r26 -> ULOZIT LOG a poslat).
