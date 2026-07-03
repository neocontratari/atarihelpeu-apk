BUILD2SA2_PS1_REAL_BOOT_FD_SEGA_WEB_ROUTING_STAGE144

DVE VECI V JEDNOM BALICKU (nezavisle oblasti):

===== A) PS1 SA2: REALNY BOOT HRY =====
- nap_ps1_native.cpp: plny libretro host - environment (system dir, pixel
  format, log), video callback (0RGB1555/RGB565/XRGB8888 -> ARGB buffer),
  worker vlakno s retro_run tempovanym podle fps jadra, generation guard.
- Hra se predava pres FILE DESCRIPTOR (/proc/self/fd): 700MB .bin se
  NEKOPIRUJE, jadro si cte sektory primo. BIOS (512KB) se uklada pres base64
  do files/ps1_system - stejne pravidlo jako ROM: NIKDY v APK.
- SA2 zamerne BEZ zvuku (sample se pocitaji a zahazuji - marker
  audioDroppedSA2) a BEZ ovladani - jedna vec najednou (RT/RU lekce).
- OBRAZ NA OBRAZOVCE: SA2 pocita snimky v bufferu (PS1_RUN frames=N res=WxH).
  Napojeni na TextureView je SA2b - kdyz frames pobezi, pixely uz jen ukazeme.

POSTUP TESTU PS1 (S8!):
1) PS1 stranka -> LOAD GAME -> vyber scph1001.bin
   -> v logu: PS1_BIOS_SAVED path=... bytes=524288
2) CD/ISO -> vybere se hra pres NATIVNI vyber (jiny nez driv!) ->
   Medal of Honor.bin nebo Crash Bandicoot.bin
3) Do logu zacne kazdou sekundu chodit PS1_RUN radek:
   - USPECH = frames ROSTE (treba frames=60,120,180...) a res=320x240
     => hra OPRAVDU BEZI v jadre na tvem telefonu, jen ji jeste nevidis.
   - Kdyz PS1_BOOT_FAIL: posli log, tam bude duvod (napr. chybejici BIOS).
4) ULOZIT LOG a poslat. Interpreter = hra pobezi pomalu, to je znamy stav,
   dynarec zrychleni je dalsi etapa.

===== B) SBIRKA: SEGA HRY UZ NE DO ATARI =====
Pricina: kazdy herni odkaz z webu sel natvrdo do Atari emulatoru.
Fix: koncovky .gen/.md/.smd/.sms/.68k/.sgd se stahnou a otevrou v EMU SEGA
(vlozeni stejne cestou jako rucni vyber ROM - marker WEB_ROM_INJECT).
Atari koncovky (xex/atr/com/exe/zip) jedou postaru.
DULEZITE PRO TVE STRANKY: Sega hry davej na web s koncovkou .gen nebo .md
(NE .zip - zip je Atari cesta).

POSTUP TESTU SBIRKA:
1) Sega stranka -> SBIRKA (kratky tap) -> klik na Sega hru (.gen)
2) Musi se otevrit EMU SEGA a hra jede; v C++ logu:
   SEGA_WEB_ROM_DOWNLOADED + SEGA_WEB_ROM_INJECTED + WEB_ROM_INJECT

ZMENENE SOUBORY:
- app/src/main/cpp/nap_ps1_native.cpp        (SA2 libretro host)
- app/src/main/cpp/CMakeLists.txt            (beze zmeny obsahu vuci FIX3 - pro jistotu)
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java (AHPS1 boot most + Sega web routing)
- app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java (boot/status/stop)
- app/src/main/assets/emu_ps1/index.html     (BIOS save + boot + status polling)
- app/src/main/assets/emu_sega/index.html    (napInjectRomBase64 vstup pro web hry)

OVERENO U ME (slibena disciplina):
- Cely napps1core vc. noveho hosta: ARM64 krizova kompilace + link
  s -Wl,--no-undefined: OK, 4 JNI symboly exportovane.
- Oba JS bloky: node --check OK. Java zavorky vyvazene.
- Kill-switch NAP_ENABLE_PS1 a arm64-only pojistka zustavaji.
POZOR: plny NDK/Gradle build probehne v Actions; kdyby cervena, screenshot staci.
