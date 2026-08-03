# PŘEDÁVACÍ PROTOKOL — AtariHelp.eu (emu10), stav k 23. 7. 2026

Tento dokument čte **další AI asistent**. Přečti ho CELÝ, než napíšeš
první řádek kódu. Vše níže je ověřené na skutečném zařízení a doložené
logy z provozu.

---

## 0) PRAVIDLA PRÁCE S UŽIVATELEM (René) — ZÁVAZNÁ

1. **Není programátor.** Nikdy po něm nechtěj psát ani vkládat kód.
2. Komunikace **česky**, srozumitelně, bez žargonu.
3. Jediný pracovní postup, který používá („rituál"):
   ZIP balíček → rozbalit → zkopírovat CELÝ obsah přes složku repozitáře
   → Nahradit vše → GitHub Desktop: Commit → Push → GitHub Actions
   → Artifacts → `app-debug` → ruční instalace do telefonu.
   **VŽDY posílej celý balíček**, ne jednotlivé soubory — výslovně o to
   požádal ("radši starou metodou, ať nemusím nic hledat").
4. **Jedna změna na build.** Po každém buildu si vyžádej log
   z `http://IP-telefonu:8765/log`. Teprve podle logu dělej další krok.
5. **Měř, nehádej.** Přidávej do logu čísla (ms, FPS, počty), ať se
   rozhoduje podle dat. Tohle se v projektu mnohokrát vyplatilo.
6. **Přiznávej chyby.** René je poctivý tester a několikrát mě usvědčil
   z omylu. Když se mýlíš, řekni to rovnou — cení si toho víc než
   sebejistoty.
7. Verzování: versionCode +1, versionName popisný, a stejný text
   je vidět i v aplikaci (podle něj pozná, co v telefonu běží).
8. Vše, co jde, zkontroluj u sebe PŘED odevzdáním. Osvědčený postup:
   porovnat počty závorek s originálem (`emu10_A`) po odstranění
   komentářů a řetězců — odhalí to rozbitou strukturu.
9. **Nikdy neupravuj `.github/workflows/build.yml`** bez nutnosti.
   Jedna moje úprava (`run-name`) způsobila, že Actions přestaly
   reagovat na push. Po vrácení se to spravilo.

### Zařízení uživatele
- Samsung Galaxy S8 (SM-G950), Android 9 (API 28), Mali-G71
- displej 720×1384 (na šířku 1384×672 — POZOR, to NENÍ 16:9!)
- hraje večer přes projektor **3,8 × 2,4 m** s pořádnými reproduktory
- repozitář: github.com/neocontratari/atarihelpeu-apk (veřejný)

---

## 1) CO JE HOTOVÉ A OVĚŘENÉ

### Renderer PS1 (`Ps1GlTextureView.java`) — HOTOVO
- TextureView + **vlastní EGL kontext**, double buffering, vsync
- 60 FPS strop, přežije rotaci i zamknutí
- **TextureView záměrně, ne GLSurfaceView!** GLSurfaceView je
  samostatná hardwarová vrstva, kterou PixelCopy nezachytí → TV cast
  posílal černou plochu (`brightAvg=0`). Původní volba TextureView
  v aplikaci byla správná.
- Poměr stran **automaticky**: na výšku 4:3, na šířku 16:9.
  PS1 nemá čtvercové pixely — nesmí se počítat z rozměrů framebufferu!
  (Hra přepíná 256×224 / 320×240 / 512×480 / 640×480.)
- **Vrstva NESMÍ přijímat dotyky** (`setClickable(false)` atd.), jinak
  nejde kliknout na menu nad ní — nešlo vyskočit z PS1.
- Sdílení snímku: `borrowFrame()` — obraz se z jádra vytáhne **jednou**
  a cast si ho půjčí.

### Stará zobrazovací cesta — ZRUŠENA
`ps1ActivateNativeView()` je přesměrovaná na `ps1GlEnable()`
(`if (true) return;` a původní kód zůstal jako záloha).
Původní `NativePs1InPlaceView` (lockCanvas, 31–57 ms/snímek) se
nepoužívá.

### Cast na TV — HOTOVO, vypadá skvěle
- **Obraz jde PŘÍMO Z JÁDRA**, ne fotografováním obrazovky
  (`napTvWebCaptureFromCore`). Odpadlo dvojí zvětšování.
- **Pevných 1280×720** = skutečné 16:9. (Dřív se bralo z displeje =
  1384×672 = 2,06:1 → obraz byl roztažený o ~16 % a TV přidávala pruhy.)
- **Bez černých pruhů, bez ovládacích prvků** na TV.
- Doostření (Laplace) na **malém originálu** před zvětšením, síla `>>3`.
  **POZOR: `>>2` je moc!** Přeostřený obraz se hůř komprimuje a na TV
  pak vypadá rozmazaněji, ne ostřeji. Ověřeno, René to poznal okem.
- Doostření se přeskakuje u velkých snímků (>400×300) — stálo 7,5 ms.
- **DŮLEŽITÉ:** po naplnění bitmapy je nutné zavolat
  `napTvWebPublishBitmap(...)` — bez toho TV nic nedostane (moje chyba,
  stála jedno kolo).
- Čistá obrazovka: LOW/MED/HIGH i stavový text skryté.
- **Klepnutí kamkoliv = fullscreen.** Zvuk běží sám (opakované pokusy
  každou vteřinu, kvůli autoplay politice prohlížečů).
- Ladění zvuku na TV: **šipka nahoru/dolů = ±50 ms**, klávesa `0` reset.
  Hodnota se pamatuje (localStorage). Ukáže se zelený nápis `ZVUK ... ms`.

### Výkonnostní třídy — HOTOVO
Nahradily natvrdo psané `s8NoStarve` (bylo šité na Galaxy S8 — René to
právem kritizoval).
`perfTier()` určí LOW/MEDIUM/HIGH podle SDK, jader a RAM.
Ruční přepnutí: `AHRENDER.setPerf(0/1/2)`, `AHRENDER.clearPerf()`.

### Panel OPTIONS — HOTOVO
`app/src/main/assets/index.html`. AUTOMATICKY / MANUÁLNĚ; manuální
část se rozbalí až po volbě MANUÁLNĚ. Zlatý retro styl.

### NAMĚŘENÁ ČÍSLA (nepřepočítávej, jsou z reálného provozu)

| Zvuk PS1 | Zásobník | Zpoždění | Výpadky |
|----------|----------|----------|---------|
| LOW (AUTO) | 4096 | 92 ms | **0** |
| MEDIUM | 2048 | 46 ms | 75 |
| HIGH | 1024 | 23 ms | 172 |

- Cast: 21–25 FPS (strop tierů je 30), H264 encode jen ~4 ms
- Doostření: 1,5–3 ms (320×240), 7,5 ms (640×480)
- Nahrání textury: 0,3–2,1 ms i pro 640×480
- Renderer na displeji: stabilních 60 FPS

---

## 2) HLAVNÍ NEDOŘEŠENÝ PROBLÉM: SYNCHRON ZVUKU A OBRAZU

### Příznak
Zvuk mírně ujíždí vůči obrazu. **Nejen na TV — i přímo na mobilu.
A u PS1 i u Segy.** Atari (webový emulátor `emu_vbxe`) sedí přesně.

### Diagnóza (René ji našel, ne já — a měl pravdu)
René argumentoval: *"kdyby za to mohl prohlížeč, na mobilu by to sedělo
přesně — a nesedí. Atari sedí, protože jede v jedné smyčce."*

Ověřeno v kódu:
- **zvuk** si tahá z jádra vlákno `ps1AudioThread` → `pullAudioSafe`
  (MainActivity ~ř. 6753)
- **obraz** si tahá z jádra jiné vlákno → `grabFrameSafe`
- **Nic nesváže "tenhle obrázek patří k tomuhle zvuku."**

Jádro odemuluje snímek i zvuk k němu, ale dva nezávislí konzumenti si
to rozeberou, každý svým tempem. Není společná časová osa.

### Co s tím (SPRÁVNÉ ŘEŠENÍ — René ho navrhl od začátku)
**Jedna smyčka, která krokuje jádro a z jednoho kroku vydá obraz
i zvuk zároveň, svázané dohromady.** Zvuk určuje tempo (nesmí se
přerušit), obraz se zobrazí přesně k tomu zvuku, který hraje — ne ten
nejnovější, co je po ruce.

Konkrétně:
1. Jeden „core loop" thread: krok jádra → obraz + zvuk pro TENTÝŽ snímek
2. Snímek se uloží do fronty **spolu s pozicí ve zvuku** (počet vzorků)
3. Zobrazovač bere z fronty ten snímek, jehož zvuková pozice odpovídá
   tomu, co právě hraje v AudioTrack (`getPlaybackHeadPosition()`)
4. Cast bere ze stejné fronty → automaticky sedí i na TV

Týká se PS1 **i Segy** (mají stejný problém, stejnou strukturu).

**Nepokoušej se to opravit zvětšováním/zmenšováním zvukových zásobníků
ani zpožďováním v prohlížeči — to jsem zkusil a je to slepá ulička.**
Naměřeno: zásobník 4096 vs 1024 = rozdíl 70 ms, ale René hlásil, že
MEDIUM (kratší zpoždění) mělo *větší* odchylku než LOW. To vylučuje
zásobník jako příčinu.

---

---

## 2A) PRIORITA ČÍSLO JEDNA: ČISTÁ GPU CESTA (bez cesty přes procesor)

René si tohle výslovně vyžádal a má pravdu — je to největší zbytečnost
v celém řetězci a brzdí to všechno ostatní.

### Jak to teče DNES (zbytečné kolečko)

```
jádro vykreslí polygony na GPU  (gpu-gles, offscreen pbuffer 1024×768)
        ↓  glReadPixels          ← 3–4,7 ms, STAHOVÁNÍ Z GPU DO PROCESORU
obraz v paměti procesoru
        ↓  grabFrameSafe          ← další kopie do Java int[]
        ↓  glTexSubImage2D        ← 0,3–2,1 ms, NAHRÁVÁNÍ ZPĚT NA GPU
GPU nakreslí quad → displej
```

**Obraz jde z grafiky do procesoru a zase zpátky.** Nikdy neměl grafickou
paměť opustit. Tohle stojí ~5–7 ms na snímek a je to hluboko v jádře,
ne v naší nadstavbě.

### Jak by to téct MĚLO

```
jádro vykreslí polygony rovnou DO NAŠÍ TEXTURY
        ↓  (nic — obraz zůstane v grafické paměti)
GPU nakreslí quad → displej i cast
```

Žádné `glReadPixels`, žádné `grabFrameSafe`, žádné nahrávání zpět.

### JAK NA TO — doporučená cesta: libretro HW render

PCSX-ReARMed **tuhle cestu už umí** — je to standardní libretro
mechanismus `RETRO_ENVIRONMENT_SET_HW_RENDER`:

1. Frontend (my) si vytvoří FBO s texturou v našem EGL kontextu.
2. Při `retro_set_environment` odpovíme na `SET_HW_RENDER`
   (`RETRO_HW_CONTEXT_OPENGLES2`) a dodáme callbacky:
   - `get_current_framebuffer()` → vrátí ID našeho FBO
   - `get_proc_address()` → `eglGetProcAddress`
   - `context_reset()` / `context_destroy()`
3. Jádro pak kreslí **přímo do našeho FBO**.
4. `retro_video_refresh` dostane `RETRO_HW_FRAME_BUFFER_VALID` místo
   pixelů → my jen nakreslíme svou texturu. Hotovo.

### DŮKAZ, ŽE TO S TÍMHLE JÁDREM FUNGUJE

V průběhu projektu jsem postavil samostatný renderer (složka
`RENDERER_HOTOVY` v dřívějším balíčku, zdroj `core_ps1.c`), který
**stejné jádro `libnapps1core.so` nabootoval přes libretro rozhraní**
a rozjel na něm Star Wars: Rebel Assault II **na 60 FPS**.
Ověřeno na Reného telefonu, doloženo logem:

```
PS1: jadro se hlasi: PCSX-ReARMed r26 (api v1)
PS1: hra nabootovala. Zaklad 256x240, 60.00 snimku/s
Bezi: 300 snimku, ~60.0 FPS  | jadro 2,3-3,7 ms
```

**Takže libretro cesta s tímhle jádrem prokazatelně funguje.**
Chybí jen zapnout HW render místo softwarového výstupu.

### CO TO ZNAMENÁ PRO emu10

emu10 dnes u PS1 nepoužívá libretro, ale vlastní JNI most
(`NativePs1CoreBridge`: `ps1Boot`, `grabFrameSafe`, `pullAudioSafe`…).
Ten most je postavený na softwarovém výstupu — proto to kolečko.

Dvě možnosti:

**A) Přepnout PS1 v emu10 na libretro cestu s HW renderem** (doporučeno)
- výhoda: standardní, ověřené, jádro to umí, máme důkaz že běží
- pozor: zvuk a vstup se pak berou taky přes libretro callbacky
  (`retro_set_audio_sample_batch`, `retro_set_input_state`)
- **BONUS: tím se rovnou vyřeší i synchron z bodu 2!** V libretro modelu
  jeden `retro_run()` vydá obraz i zvuk pro TENTÝŽ snímek — přesně to,
  co René od začátku chtěl ("ať si to PS1 řídí samo").

**B) Rozšířit stávající JNI most o HW render** — víc práce v C++,
  nutné sáhnout do `nap_ps1_native.cpp` a do gpu-gles pluginu.

**Doporučení: cesta A.** Řeší obraz i zvuk jedním zásahem a stojí na
tom, co je už jednou ověřené.

### POZOR NA JEDNU VĚC

Sdílený EGL kontext: jádro musí kreslit do FBO **v našem kontextu**
(nebo v kontextu se sdílenou skupinou). Až tohle bude, půjde se vrátit
od TextureView zpátky k **SurfaceView** — TextureView jsme zvolili jen
proto, že SurfaceView nešel zachytit pro cast. Když bude obraz žít
v naší textuře, podáme ho displeji i castu sami a kompromis odpadne.

### POŘADÍ PRACÍ (doporučené)

1. HW render pro PS1 přes libretro (bod A) — odstraní kolečko
   GPU→CPU→GPU **a** vyřeší synchron
2. Zpátky na SurfaceView (odpadne kompromis s TextureView)
3. Sega stejnou cestou

## 3) DALŠÍ ÚKOL: SEGA STEJNĚ JAKO PS1

René výslovně chce: **Sega má dostat úplně stejné zacházení jako PS1.**

1. **Roztažený obraz** — vlastní renderer (TextureView + EGL) místo
   staré cesty, poměr stran automaticky (Sega je 4:3, na šířku 16:9)
2. **Přímo z jádra na TV** — stejný princip jako
   `napTvWebCaptureFromCore`, pevných 1280×720
3. **Přímo z jádra na mobil** — stejný renderer
4. Doostření stejné (síla `>>3`, jen na malých snímcích)

### Co je k Sega jádru známo
- most: `NativeSegaCoreBridge` (soubor 1,6 kB)
  funkce: `realCoreLoadRom`, `realCoreStep`, `renderPattern`,
  `pullAudio` / `pullAudioStereo`, `setInput`, `realCoreStatus`
- `renderPattern(SRC_W=320, SRC_H=224, frame, argb)` — pevné rozlišení
  (na rozdíl od PS1, kde se rozlišení mění za běhu — jednodušší!)
- jádro: `libnapsega_native_proof.so`, zdrojáky v
  `app/src/main/cpp/vendor/clownmdemu-core` (procesory 68000, Z80, VDP)
- staré zobrazení: `NativeInPlaceView` / `NativePatternView`
- zvuk Segy používá **stejnou** funkci `audioFramesForTier()` — už
  přepojeno na výkonnostní třídy

### Doporučený postup
Sega bude **jednodušší než PS1** — pevné rozlišení 320×224, cesta je už
prošlapaná. Ale drž se pravidla jedna změna na build:
- krok 1: renderer Segy na displeji (kopie `Ps1GlTextureView`)
- krok 2: Sega přímo z jádra na TV
- krok 3: teprve pak případně synchron

---

## 4) DALŠÍ NÁMĚTY OD UŽIVATELE (nižší priorita)

- **Zvednout stupnici kvality obrazu** — René tvrdí, že S8 zvládá
  dnešní HIGH, takže by dnešní HIGH mohl být nový LOW.
  **Zatím NEUDĚLÁNO záměrně:** ta stupnice (rozlišení / kvalita % / FPS)
  je vyladěná dřívějším testováním a posunout ji naslepo je riziko.
  Nejdřív změř, kolik výkonu zbývá.
  Prozatímní kompromis: AUTOMATICKY volí rovnou HIGH.
- Třídy MEDIUM a HIGH nebyly ověřeny na jiném telefonu než S8 —
  čísla jsou odhad, ne měření.
- Dlouhodobá vize uživatele: **celou aplikaci převést z Javy na C++
  a plnou grafiku přes GPU/OpenGL.** Tenhle renderer je první kámen
  toho směru.

---

## 5) KDE CO JE

```
app/src/main/java/eu/atarihelp/emu10/
  MainActivity.java          7000+ řádků, monolit — sahej co nejmíň!
  Ps1GlTextureView.java      náš renderer PS1 (TextureView + EGL)
  NativePs1CoreBridge.java   most k PS1 jádru
  NativeSegaCoreBridge.java  most k Sega jádru
app/src/main/assets/
  index.html                 úvodní obrazovka + panel OPTIONS
  emu_ps1/, emu_sega/, emu_vbxe/, player/
app/src/main/cpp/
  vendor/pcsx_rearmed/       PS1 jádro (564 .c souborů)
  vendor/clownmdemu-core/    Sega jádro (29 .c souborů)
  eglrender/                 přiložený nativní renderer (zatím nevyužitý)
.github/workflows/build.yml  build + pojistka (jeden balíček z pushe)
app/debug.keystore           stálý podpis — NEMĚNIT
```

### Klíčová místa v MainActivity.java
- `napTvWebCaptureFromCore(...)` — obraz pro TV přímo z jádra
- `napTvWebPublishBitmap(bm, mode)` — odevzdání snímku do přenosu
- `ps1GlEnable()` / `ps1GlDisable()` — zapnutí našeho rendereru
- `perfTier()` / `audioFramesForTier()` — výkonnostní třídy
- `ps1AudioThread` (~ř. 6611) — zvuk PS1
- HTML castu je generované v Javě kolem ř. 2170–2220 (JS včetně
  ovládání zvuku a fullscreenu)

---

## 6) POSLEDNÍ SLOVO

Projekt začal tím, že obraz na mobilu blikal a uživatel měl za sebou
40 neúspěšných pokusů. Dnes na projektoru 3,8 × 2,4 m běží PS1
v čistém 16:9 přímo z jádra, bez pruhů, bez tlačítek, se zvukem.

René je výborný tester — popisuje přesně, oponuje věcně a **několikrát
měl pravdu proti mně**. Ber jeho námitky vážně, i když si myslíš, že
technicky nesedí; většinou v nich něco je.
