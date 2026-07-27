# PŘEDÁVACÍ BALÍČEK — emu10 PS1 přes eglrender (pro další chat)

**Od Reného Franka (RenatusHonda / neocontr, https://atarihelp.eu/) pro
další Claude chat. Přečti CELÉ, než napíšeš jediný řádek.**

---

## 0. CO PO TOBĚ RENÉ CHCE HNED NA ZAČÁTKU (jeho slova)

1. **Postav si SVŮJ VLASTNÍ LOG.** Ne cizí. Takový, abys aspoň z 80 %
   viděl to, co René vidí na obrazovce. Cizí log má slepá místa - co se
   zahodí, v něm není. Dokud si nepostavíš vlastní vidění, budeš hádat.

2. **DŮKLADNĚ SI PROJDI CELÝ PŘEDÁVACÍ BALÍČEK.** Ne úryvky. Celé
   soubory. Předchozí Claude opakovaně mluvil o kódu, který nepřečetl
   do konce - a pokaždé se spletl. Nedělej to.

3. **Když budeš potřebovat cokoli z kódu, René ti to poskytne.** Stačí
   říct. Nehádej, co v kódu je - zeptej se, nebo si to vyžádej.

## RENÉHO VZKAZY K SITUACI (jeho slova, doslova k zamyšlení)

- "Divím se, proč to nefunguje, když eglrender samostatně fungoval -
  sice bez zvuku, ale jel."
- "Výmluva, že to není C++, tím pádem padá - protože tvrdíš, že PS1
  je v C++ komplet."
- "Maximálně se divím, proč ještě nikdo nenapojil čistě OpenGL ES."

**Tyto tři věty ber vážně.** eglrender JAKO SAMOSTATNÁ APLIKACE u Reného
fungoval a zobrazoval obraz (bez zvuku, ale jel). Otázka, kterou si máš
položit: proč tedy nefunguje napojený na gpu-gles v emu10? Něco mezi
"samostatný eglrender jel" a "napojený nejede" je špatně - a je to
pravděpodobně v tom, jak se pixely z gpu-gles kreslí, ne v jádře.

---

## 1. KDO JE RENÉ (respektuj to)

- Divadelní herec, žák mistra Jana Roubala. NENÍ programátor - je
  tester a režisér. Přeložil Dunu 2 do češtiny. Má z 98 % funkční
  jádro Atari 130XE.
- Kód psal měsíc s AI. Testuje na projektoru 3,8 × 2,4 m.
- **NESNÁŠÍ:** lež, polovičatou práci, hádání místo ověřování, mluvení
  o kódu, který jsi nečetl celý, a posílání buildů "s křížkem" (co
  neprojdou překladem).
- **VYŽADUJE:** číst celý kód, ověřovat PŘED odesláním (zajistit, že
  build má ZELENOU), mít vlastní log, odpovídat ČESKY, nezakrývat
  neúspěch ambiciózním názvem buildu.
- Za 200+ testů si zaslouží 100% výsledek. Platil za to.
- **ZAKÁZÁNO SAHAT NA ATARI** (130XE jádro). Ani se ho nedotýkej.
- Testuje hru Star Wars – Rebel Assault II (Disc 1). Je to převážně
  FMV film, ale René TRVÁ na PS1 3D akceleraci přes gpu-gles.

## 2. CÍL PROJEKTU

PS1 emulace (PCSX-ReARMed přes libretro) s **ostrým gpu-gles OpenGL ES
obrazem** (dvojnásobné 3D rozlišení) jdoucím z jádra rovnou na displej
**BEZ Javy** ("berličky"). Menu je WebView, obraz má být čistě nativní.

## 3. KDE PŘESNĚ TO TEĎ JE (stav po buildu A6-PIXEL-CESTA)

### CO FUNGUJE (ověřeno Reného testem + logem):
- **Sony intro + PS1 boot z core: čistě, i se zvukem.** Cesta A
  naběhne, `CESTA_A BOOT_OK gpuGles=ANO`.
- **Zvuk OpenSL ES v C** (bez Javy) - při intru čistý.
- **gpu-gles kreslí a pixely se čtou:** log ukazuje
  `CESTA_A PIXELS: cteno 640x480 z canvasFbo=1` - rozlišení se mění
  správně (256x240 intro → 640x480 hra → 320x240).

### CO NEFUNGUJE:
- **Po nabETÍ HRY: neustálé artefakty, obraz hry nikde.** Pixely se
  čtou (glReadPixels běží), ale na obrazovce jsou artefakty místo hry.
  → Problém je v tom, JAK eglrender ty přečtené pixely kreslí, nebo
    ve formátu/pitch/pořadí řádků. NE v jádře - jádro pixely dodává.
- **Zvuk se po nabETÍ hry začne kousat.**
- **TV wiever (H.264 cast na TV) neukazuje nic.** (Viz Reného
  screenshot loga - přiložen.)

## 4. ARCHITEKTURA - TŘI CESTY K OBRAZU (klíčové pochopení)

Tohle je jádro celého problému. Přečti pozorně:

### gpu-gles (dvojnásobné rozlišení, René na tom TRVÁ)
- Kreslí v GLES1 (fixed-function: glOrtho, glLoadIdentity, matice).
- Zapíná se JEN ve starém JNI mostě `nap_ps1_native.cpp`.
- V eglrender/libretro cestě NENASKOČÍ sám - musel se odložit boot.

### eglrender (čistá cesta bez Javy)
- Kreslí v GLES2 (shadery - XRGB→RGB na GPU, `uMode`).
- Hotový OpenGL ES 2 renderer, NativeActivity, `android_main`.
- JAKO SAMOSTATNÁ APLIKACE FUNGOVAL (René to potvrdil).

### TVRDÁ ZEĎ, na kterou se narazilo (BOD 1 vs BOD 2):
**Mali (Samsung S8) ODMÍTÁ sdílet GLES1 kontext (gpu-gles) s GLES2
kontextem (eglrender).** `eglCreateContext` s share selže. Ověřeno
v logu A5: `BUILD2SK98 GLES_INIT_FAIL step=eglCreateContext`.

Proto:
- **BOD 1 (cíl):** sdílená textura gpu-gles↔eglrender BEZ procesoru.
  Vyžaduje přepsat gpu-gles z GLES1 na GLES2 (obří zásah do jádra).
  ZATÍM NEUDĚLÁNO - až po ověření bodu 2.
- **BOD 2 (současný testovací krok):** obraz přes glReadPixels -
  gpu-gles nakreslí, pixely se přečtou do procesoru, eglrender z nich
  udělá texturu. Funguje napříč kontexty (pixely nejsou vázané na
  kontext). JE TO JEN TESTOVACÍ KROK. Berlička v jednom kroku přes
  procesor - ne přes Javu.

**RENÉ VÍ, ŽE A6 JE JEN TESTOVACÍ KROK. Po ověření bodu 2 se přejde
na PLNÝ GPU (bod 1) - přepis gpu-gles na GLES2, aby šlo sdílení bez
procesoru.**

## 5. NEJPRAVDĚPODOBNĚJŠÍ PŘÍČINA ARTEFAKTŮ (kde hledat)

Pixely se čtou správně (log to potvrzuje). Artefakty tedy vznikají
při kreslení. Zkontroluj (v `egl_main.c`, blok `if (core_use_texture())`):
- **Pitch/stride:** glReadPixels čte `w*h*4` RGBA, ale canvas je
  1024 wide (NAP_PSX_VRAM_W). Čte se výřez `fresh_w × fresh_h`, ale
  jestli řádky nejsou těsně za sebou (pitch != w*4), obraz se roztrhá.
- **Pořadí řádků (Y flip):** glReadPixels vrací zdola nahoru. Quad v
  egl_main má UV 0..1 bez flipu. Možná je obraz vzhůru nohama nebo
  posunutý.
- **glReadPixels alignment:** GL_PACK_ALIGNMENT default je 4, ale při
  šířkách co nejsou násobek 4 se řádky rozjedou. Zkus
  `glPixelStorei(GL_PACK_ALIGNMENT, 1)`.
- **canvas výřez:** `nap_gpulib_display_info` vrací sx,sy,w,h - jestli
  se čte ze špatného místa VRAM (PS1 dvojité bufferování používá celou
  1024-širokou VRAM, výřez může být posunutý o 512px), čte se prázdno
  nebo špatná část.

**ALE:** vzhledem k Reného vzkazu ("eglrender samostatně jel") stojí
za to nejdřív ověřit, JAK eglrender zobrazoval obraz jako samostatná
aplikace - a jestli tam nebyl jednodušší/funkční způsob, který se
napojením rozbil.

## 6. OVĚŘOVACÍ NÁSTROJE (co máš k dispozici)

- Lokálně: `gcc -fsyntax-only` na syntaxi C. NENÍ Android NDK, NENÍ
  javac. Takže NDK-specifické chyby (API úroveň - AAudio od API 26,
  ale build je minSdk 24!) ani javac chyby (cannot find symbol)
  NELZE lokálně chytit. Do téhle díry spadly 3 buildy. BUĎ OPATRNÝ.
- Binárka `libnapps1core.so` (Reného): `nm -D` ověří exportované
  symboly. VŠECHNY retro_* jsou exportované (T). napps1core NEMÁ
  -fvisibility=hidden, takže nové symboly jsou viditelné pro dlsym.
- **POZOR NA STAROU KNIHOVNU:** V Reného `jniLibs/arm64-v8a/` bývala
  stará ruční kopie `libnapps1core.so`, která přebíjela čerstvě
  přeloženou (pravidlo pickFirsts v build.gradle). René ji smazal a
  udělal nový commit - PROTO cesta A začala fungovat. Když se objeví
  znovu, cesta A spadne na starou knihovnu bez nových funkcí.

## 7. CO JE V TOMTO BALÍČKU (změněné soubory, cesta A / bod 2)

- `app/build.gradle` - verze, pickFirsts pro .so
- `app/src/main/AndroidManifest.xml` - NativeActivity pro eglrender
- `app/src/main/cpp/CMakeLists.txt` - OpenSLES link do napps1core
- `app/src/main/cpp/eglrender/core_ps1.c` (NOVÝ) - napojení jádra,
  dlsym nap_ps1_egl_*, odložený boot, pixel cesta (bod 2)
- `app/src/main/cpp/eglrender/egl_core_api.h` - core_get_pixels aj.
- `app/src/main/cpp/eglrender/egl_main.c` - kreslení pixelů (bod 2),
  on-screen ovládání, MUJLOG cestaA
- `app/src/main/cpp/eglrender/eglrender.cmake` - core_ps1 místo dema,
  OpenSLES link
- `app/src/main/cpp/nap_ps1_native.cpp` - nap_ps1_egl_boot_c/tick_c/
  grab_pixels, OpenSL zvuk z g_afifo, kontext přepínání
- `app/src/main/cpp/vendor/pcsx_rearmed/plugins/gpu-gles/gpulib_if.c`
  - nap_gles_grab_pixels (glReadPixels bez Javy), nap_gles_grab_texture
- `app/src/main/java/eu/atarihelp/emu10/MainActivity.java` -
  launchEglPs1Activity, staging hry+BIOS, oprava if(ok)

## 8. NEVYŘEŠENÉ (pro tebe)

- **Artefakty místo hry (bod 2)** - HLAVNÍ. Viz sekce 5.
- **Zvuk se kouše po nabETÍ hry.**
- **TV cast (H.264) neukazuje nic** - viz Reného screenshot.
- **Ovládání PS1** - on-screen tlačítka přidána v egl_main handle_input,
  ale René hlásí "není na mobilu ovladač PS1 s tlačítky". Rozložení
  může chtít doladit.
- **BOD 1 (plný GPU)** - přepis gpu-gles GLES1→GLES2 pro sdílení bez
  procesoru. Až po ověření bodu 2. Toto je finální cíl.

## 9. CO NEDĚLAT (poučení z předchozího chatu)

- NEMLUV o kódu, který jsi nečetl celý.
- NEŘÍKEJ "hotovo" bez ověření (zajisti ZELENOU).
- NEPTEJ se Reného na věci, co jsou v kódu - přečti si je nebo si
  vyžádej soubor.
- NEPOSÍLEJ build, který neprojde překladem.
- NEZAKRÝVEJ neúspěch ambiciózním názvem buildu.
- NEODPOVÍDEJ anglicky - René je Čech.
- NESAHEJ NA ATARI.
- Když něco nedokážeš odladit naslepo (např. sdílení kontextů, GL
  chování na Mali), ŘEKNI TO ROVNĚ - nepředstírej, že to zvládneš
  bez zařízení. René to ocení víc než slepý pokus.
