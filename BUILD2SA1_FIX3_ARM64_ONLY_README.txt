BUILD2SA1_FIX3_ARM64_ONLY_STAGE143D - JEDEN SOUBOR (CMakeLists.txt)

PRICINA 3. CERVENE (z tveho screenshotu): Gradle stavi APK pro VSECHNY
architektury procesoru. Prvni pad byl arm64 (zlib - opraveno ve FIX2),
tenhle pad je armeabi-v7a - stara 32bit vetev, kde ma jadro uplne jinou
konfiguraci (assembly misto C intrinsics). Ja mel PS1 od zacatku omezit
na arm64. Treti chyba, treti priznani.

OPRAVA: PS1 jadro se stavi POUZE pro arm64-v8a (tvoje S8 a vsechny moderni
telefony). Ostatni architektury PS1 preskoci se statusem v logu a Sega se
na nich stavi presne jako dosud. Zadna architektura uz nemuze shodit build.

CO TO ZNAMENA PRAKTICKY:
- S8: PS1 core plnohodnotne (arm64).
- Nox (x86 s ARM prekladem): PS1 stranka muze hlasit PS1_CORE_LOAD_FAIL -
  to je ocekavane a poctive; PS1 testujeme primarne na S8. Sega v Noxu bezi dal.

OVERENO U ME (nove, poctive):
- Vsech 77 PS1 zdrojaku zkompilovano KRIZOVYM ARM64 kompilatorem
  a slinkovano s -Wl,--no-undefined do skutecne aarch64 .so: OK.
- x86_64 build + realne volani jadra: PS1_CORE_ALIVE name=PCSX-ReARMed r26.
- Kill-switch NAP_ENABLE_PS1 porad k dispozici (ON -> OFF = zelena bez PS1).

POSTUP: commit tohoto jednoho souboru -> Actions.
