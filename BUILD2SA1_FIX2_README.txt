BUILD2SA1_FIX2_SYSTEM_ZLIB_LINK_STAGE143C - OPRAVA LINKOVANI (1 SOUBOR)

PRICINA CERVENEHO BUILDU #853 (nasel jsem ji z tveho screenshotu, nemusis
nic hledat): jejich Android.mk ma radek "LOCAL_LDLIBS := -lz -llog" -
PS1 jadro potrebuje systemovou knihovnu zlib (libchdr = komprese CD obrazu).
Ja pri prechodu na Android.mk seznam zdrojaku tenhle radek neprevzal
=> linker nenasel inflate/crc32 => ninja spadl. Moje chyba, uznavam druhou.

OPRAVA: JEDEN soubor - app/src/main/cpp/CMakeLists.txt:
- napps1core nove linkuje systemovou zlib (z) + dl (NDK je ma vzdy k dispozici)
- pridany -Wl,--no-undefined: STEJNA prisnost jako NDK. Od ted kazdy chybejici
  symbol shodi uz MUJ test u me, ne tvuj workflow. Tohle je systemova pojistka,
  ne jednorazovy flastr.
- pridany -Wl,-z,max-page-size=16384 (prevzato z Android.mk, kompatibilita
  s novymi Androidy, S8 nevadi)
- KILL-SWITCH NAP_ENABLE_PS1 zustava: kdyby COKOLI, ON -> OFF = zeleny workflow.

OVERENO U ME S NDK PRISNOSTI:
- Cely napps1core slinkovan s -Wl,--no-undefined a systemovou zlib: OK
- Realne volani jadra: PS1_CORE_ALIVE api=1 name=PCSX-ReARMed version=r26

POSTUP: commit tohoto jednoho souboru -> Actions. Kdyz zelena: PS1 stranka
-> SETTINGS -> LOG -> radek PS1_CORE_PROBE PS1_CORE_COMPILED ... -> ULOZIT LOG.
Kdyz cervena: prepni kill-switch na OFF (zelena hned) a posli mi jen screenshot
cerveneho kroku jako ted - to bohate staci, chybu si najdu sam.
