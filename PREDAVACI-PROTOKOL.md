# PŘEDÁVACÍ PROTOKOL – projekt AH EGL Render → AtariHelp.eu (emu10)

Tento dokument je určen dalšímu AI asistentovi (Claude, GPT, cokoliv).
Přečti ho CELÝ, než napíšeš první řádek kódu. Vše níže je ověřené
na skutečném zařízení a doložené logy.

---

## 0) PRAVIDLA PRÁCE S UŽIVATELEM (závazná)

1. Uživatel NENÍ programátor. Nikdy po něm nechtěj psát ani vkládat kód.
2. Komunikace česky, srozumitelně, bez žargonu.
3. Jediný pracovní postup („rituál"), který uživatel umí:
   ZIP balíček → rozbalit → zkopírovat obsah přes složku repozitáře →
   GitHub Desktop: Commit → Push → GitHub Actions postaví APK →
   Artifacts → app-debug → ruční instalace do telefonu.
4. V repozitáři smí být JEDEN workflow a z jednoho pushe smí vzniknout
   JEDEN balíček. Workflow `.github/workflows/build.yml` obsahuje
   „Pojistku", která cizí workflow sama vypíná. NIKDY nepřidávej druhý
   workflow soubor, pojistku zachovej.
5. JEDNA změna na build. Po každém buildu si vyžádej log z
   `http://IP-telefonu:8765/log` (aplikace emu10 musí být úplně
   vypnutá – drží stejný port). Teprve podle logu dělej další krok.
6. Podpisový klíč `app/debug.keystore` je součást repa – NEMĚNIT,
   díky němu jdou verze instalovat přes sebe.
7. Každou verzi: versionCode +1, versionName popisný, a stejný text
   vypiš i ve startovním logu (`main.c`, řádek „AH EGL Render start").
8. Vše, co jde, zkompiluj/otestuj u sebe PŘED odevzdáním
   (gcc -Wall -Wextra -Werror; viz dosavadní praxe).
9. Uživatel má omezený rozpočet – neplýtvej koly. Buildy na GitHubu
   jsou zdarma, konzultace AI ne.

## 1) CO JE HOTOVÉ A OVĚŘENÉ (s čísly z logů zařízení)

Zařízení: arm64, Android 9 (API 28), GPU Mali-G71, displej 720×1384.

- Čistě nativní renderer (NativeActivity, C, EGL + OpenGL ES 2.0):
  RGBA8888 config, explicitní BACK buffer (double buffering),
  eglSwapInterval(1) = vsync. Stabilních 60,0 FPS dlouhé minuty.
- Přežití rotace ZA LETU (720×1384 ↔ 1384×720 bez reinicializace),
  přežití zamknutí/odemknutí (plný teardown + rebuild včetně textury
  a shaderů), čistý exit i restart. Nula chyb v lozích.
- Trasa obrazu: framebuffer → glTexSubImage2D → quad → swap.
  Naměřeno: nahrání 0,3–2,1 ms (i 640×480), kreslení 0,2–0,4 ms,
  volná rezerva ~10–13 ms z rozpočtu 16,6 ms. POZOR na optický klam:
  ovladač Mali si čekání na vsync vybírá už v prvním GL příkazu
  snímku („pozadí"), ne jen ve swapu – proto je v logu rozpad časů.
- Dynamická přestavba textury za běhu: 256×224 / 320×240 / 512×240 /
  640×480; formáty RGBA8888, XRGB8888 (prohození kanálů dělá shader,
  uniform uMode) a RGB565. Letterbox se přepočítává sám.
- Mini HTTP log server v C na portu 8765 (`/log`), kruhová paměť
  ~400 řádků. Když port drží emu10, jen to zaloguje a jede dál.
- Verze 1.4-PS1-NABEH (v tomto balíčku): přibalené SKUTEČNÉ PS1 jádro
  uživatele (`libnapps1core.so` z jeho emu10 APK = PCSX-ReARMed)
  a napojení přes standardní libretro protokol (viz core_ps1.c):
  dlopen → retro_set_environment (RGB565 dohodnuto, log interface,
  system/save dir) → retro_init → hledání hry v
  /storage/emulated/0/Download/AtariHelp/PS1 (i podsložky gdrive_*)
  → retro_load_game → retro_run každý snímek. Zvuk a vstup zatím
  záměrně zahozeny (stub callbacky). Fallback: demo vzor + přesný
  důvod v logu. TATO VERZE JEŠTĚ NEBYLA OTESTOVÁNA NA ZAŘÍZENÍ.

## 2) SOUBORY A ROLE

- `app/src/main/cpp/main.c` – EGL init/teardown, hlavní smyčka
  (ALooper_pollOnce), upload textury (respektuje pitch, po řádcích
  když pitch != width*bpp), letterbox, rozpad měření časů, obsluha
  INIT/TERM_WINDOW a focusu, reinicializace po chybě swapu.
- `app/src/main/cpp/logserver.c/.h` – HTTP log na 8765.
- `app/src/main/cpp/core_api.h` – „zásuvka": jádro dodává ukazatel +
  width + height + pitch + formát. Nic víc renderer nepotřebuje.
- `app/src/main/cpp/core_demo.c` – záložní vzor (demo_step/
  demo_get_frame), střídá 4 rozlišení/3 formáty po ~7 s.
- `app/src/main/cpp/core_ps1.c` – libretro napojení PCSX-ReARMed.
- `app/src/main/jniLibs/arm64-v8a/libnapps1core.so` – JÁDRO UŽIVATELE
  (vytaženo z jeho app-debug.apk). abiFilters = jen arm64-v8a.
- `.github/workflows/build.yml` – pin: Java 17, Gradle 8.7, AGP 8.5.2,
  NDK 26.3.11579264, CMake 3.22.1 + POJISTKA + upload artifact
  `app-debug`. Záměrně BEZ gradle wrapperu (častý zdroj pádů).
- `app/debug.keystore` – stálý podpis (PKCS12, heslo `android`).
- `README-NAVOD.md` – návod pro uživatele.

## 3) OKAMŽITÝ STAV / PRVNÍ KROKY PRO TEBE

a) Uživatel hlásí, že v Nastavení není přepínač oprávnění Úložiště.
   VYSVĚTLENÍ: přepínač se objeví AŽ po instalaci verze 1.4 (starší
   verze oprávnění nedeklarovaly). První úkol tedy: nechat uživatele
   projet rituál s tímto balíčkem, nainstalovat, POVOLIT úložiště
   (Nastavení → Aplikace → AH EGL Render → Oprávnění), spustit,
   poslat log. Ve startu logu musí být „verze 1.4-PS1-NABEH".
b) Kdyby přepínač nebyl ani u v1.4: přidej runtime žádost o oprávnění
   přes JNI (ANativeActivity->clazz, metoda requestPermissions,
   API 23+), NEBO přejdi rovnou k hlavnímu úkolu níže (v emu10 už
   oprávnění je).
c) V logu pak hledej: „jadro se hlasi" (název+verze PCSX-ReARMed),
   „bootuji hru", „hra nabootovala", řádky „PS1c:" (log samotného
   jádra) a hodnotu „jadro X ms" v řádcích „Bezi:" – to je skutečná
   cena emulace (očekávej jednotky ms; rozpočet 16,6).

## 4) HLAVNÍ ÚKOL: PŘESAZENÍ DO PŮVODNÍ APLIKACE emu10

Cíl slíbený uživateli: renderer má nahradit blikající zobrazovací
cestu v jeho aplikaci AtariHelp.eu (eu.atarihelp.emu10, workflow
„AtariHelp_eu_EMU10_BUILD1"). V emu10 dnes PS1 kreslí: GLES pbuffer
(offscreen) → glReadPixels (3–4,7 ms) → Java View. Sega: JNI →
Java View. Obojí bez vsync page-flipu → blikání. Náhrada = tento
renderer (window surface + swap), jádra zůstávají.

Postup po JEDNOM buildu:
1. ZÍSKAT ZDROJÁK emu10: je v git historii repa
   github.com/neocontratari/atarihelpeu-apk. Poslední stav před
   přepsáním = commit `d20e7bd` (běh #1076,
   „PREDAVACI_BALICEK_2026-07-22_SK153/154", 22.7.2026 ráno).
   Postup pro neprogramátora: na GitHubu otevřít tento commit →
   „Browse files" → zelené tlačítko „Code" → „Download ZIP".
   Doporučení: založit pro emu10+renderer NOVÝ repozitář (čistý
   start), přenést do něj pojistkový workflow z tohoto balíčku
   (přizpůsobit názvu modulu emu10).
2. Build emu10 beze změn → musí být zelený (baseline).
3. Přidat cpp renderer jako novou nativní knihovnu do emu10
   (CMake/ndk stejně jako zde). Nejjednodušší cesta integrace:
   nová Java aktivita (nebo stávající) se SurfaceView →
   `ANativeWindow_fromSurface(env, surface)` → předat oknu stejnou
   egl_init/draw_frame logiku z main.c (kód je na to připraven,
   pracuje s ANativeWindow*). Alternativa: ponechat NativeActivity
   jako druhou aktivitu emu10.
4. Přepnout PS1 výstup: uvnitř emu10 běží jádro ve stejném procesu →
   core_get_frame() = přímo buffer, který dnes plní jejich cesta
   (nebo znovu libretro cestou jako v core_ps1.c – funguje).
   Vypnout starou cestu pbuffer+readPixels+Java View.
5. Zvuk: retro_set_audio_sample_batch → jejich stávající audio
   (v emu10 funguje, underruns=0), nebo AAudio.
6. Vstup: dotyky → retro_set_input_state (RETRO_DEVICE_JOYPAD),
   případně jejich ps1SetInput.
7. Sega stejnou zásuvkou (JNI most NativeSegaCoreBridge:
   realCoreLoadRom / realCoreStep / renderPattern / pullAudio /
   setInput – viz INTEL-EMU10.md).
8. TV: uživatelův „AHTV WEB CAST" žije v Javě emu10 a čte framebuffer
   jádra, ne obrazovku → přesazením rendereru se nesmí rozbít; ověř.

## 5) FAKTA A ČÍSLA (nepřepočítávej znovu, jsou změřená)

- Rozpočet snímku 16,6 ms @60 Hz. PS1 jádro: 2,5–5 ms/tik (změřeno
  v emu10). Upload textury ≤2,1 ms. Rezerva několikanásobná.
- Hry: uživatel je stahuje ze svých stránek atarihelp.eu
  (PS1: ?page_id=1048, Sega/Atari: ?page_id=207 a ?page_id=1003)
  přes Google Drive odkazy; emu10 je ukládá do
  /storage/emulated/0/Download/AtariHelp/<SYSTÉM>/gdrive_<id>/.
  Ověřený titul: Crash Bandicoot (.cue+.bin).
- BIOS PS1: na webu jako PS1-BIOS_.zip, ale NENÍ nutný (jádro má
  HLE; sám uživatel na webu píše „v mé apce ho nepotřebujete").
- Port 8765 sdílí emu10 (Java server) i tento renderer (C server) –
  vždy běží jen ten, kdo ho obsadil první; pro čtení logu rendereru
  musí být emu10 vypnuté.

## 6) ŠABLONA TESTOVACÍHO PROTOKOLU (po každém buildu)

1. Actions zelené, artifact `app-debug`, instalace přes starou verzi.
2. Startovní log obsahuje očekávanou verzi.
3. Obraz: plynulý, ostrý, bez blikání; rotace + zamknutí přežije.
4. „Bezi:" řádky: FPS ~60, rozpad časů v rozpočtu.
5. Uživatel pošle text z 8765/log → teprve pak další změna.

Hodně štěstí. Základ je neprůstřelný – stavěj po jednom kroku
a nikdy neber uživateli jeho rituál.
