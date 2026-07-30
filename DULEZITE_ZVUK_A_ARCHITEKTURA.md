# DŮLEŽITÉ: co opravilo zvuk (a jak je to postavené)

**Nemazat.** Tohle je záznam toho, co se dlouho hledalo, ať se ta chyba
nevrátí při dalších úpravách.

Platí od verze **EMU10-B20-EMULACE-VLASTNI-VLAKNO** (versionCode 68).

---

## 1. Co zvuk kazilo

**Emulace běžela na vlákně, které kreslí.**

V cestě A se krok emulace (`retro_run()`) volal z `draw_frame()` v
`eglrender/egl_main.c` přes `core_step()`. To je vlákno, které zároveň
kreslí a čeká na vsync. Důsledek: cokoli zdrželo kreslení, zdrželo i
emulaci — a s ní výrobu zvuku. Zvuková fronta doběhla a doplnila se tichem.

Projevovalo se to jako **kousání závislé na náročnosti grafiky**, nejvíc
v menu a na začátku hry.

Ve staré (Javové) cestě běžela emulace na vlastním vlákně `nap_worker()`
a zvuk fungoval. Při přechodu na cestu A se emulace omylem přesunula do
vlákna s kreslením — a to byl ten regres.

## 2. Jak je to teď

- **`nap_core_thread_fn()`** v `nap_ps1_native.cpp` = vlastní vlákno emulace.
  Drží si kontext jádra, volá `retro_run()`, dokreslí snímek a přečte ho
  do jednoho ze **dvou střídajících se bufferů** (`g_frame_buf[2]`,
  hotový index v `g_frame_ready`).
- **`nap_ps1_egl_tick_c()`** už nekrokuje. Jen při prvním volání spustí
  vlákno emulace.
- **`nap_ps1_egl_grab_pixels()`** už nesahá na GL ani nepřepíná kontext —
  vrátí poslední hotový snímek z bufferu.

### Pravidlo, které se nesmí porušit

**Krok emulace nikdy nevolat z vlákna, které kreslí nebo čeká na vsync.**
Když se to poruší, zvuk začne kousat úměrně zátěži grafiky.

## 3. Detail, na kterém to spadne, když se přehlédne

Inicializace grafiky (`nap_gles_egl_init()`) si vezme **kontext jádra**,
aby v něm mohla vytvořit shadery, FBO a textury (to je správně — musí
patřit kontextu jádra). Ale **musí ho na konci uvolnit**: jeden EGL kontext
nemůže být „current" na dvou vláknech současně, takže by si ho vlákno
emulace nepřevzalo a **nekreslilo by se vůbec nic**.

Na konci inicializace se proto vrací vláknu kreslení jeho vlastní kontext
a kontext jádra se uvolní.

## 4. Zvuková cesta (co u ní zůstalo a proč)

Tyhle věci byly opraveny cestou a mají zůstat:

| Co | Proč |
|---|---|
| **Kruhová fronta bez zámku** (`g_aring`) | Dřív byl vektor pod jedním zámkem: emulace do něj vkládala (a při zvětšení se **celý kopíroval**), zvukový callback z něj mazal **zepředu** (posun celé paměti). Obojí pod jedním zámkem → zvuk čekal na grafiku. |
| **Držení fronty kolem 90 ms** | Když se nechala růst ke stropu, latence narostla a pak se **jednorázově zahodil velký kus** — a to je slyšet. |
| **Vynechání kroku emulace při přebytku** | Hra na 50 Hz na 60Hz displeji vyrobí o 20 % víc zvuku. Vynechá se každý šestý krok. **Nikdy neřešit uspáním na vlákně kreslení** — bije se to s vsyncem a rozkmitá se to (zpomalí/zrychlí). |
| **8 → 4 bloky OpenSL, každý vlastní paměť** | Původně se do fronty zařazoval **jeden a tentýž blok dvakrát** — přehrávalo se z paměti, kterou callback zároveň přepisoval. |
| **Doznívání místo tvrdého ticha** | Při krátkém výpadku je to slyšet mnohem míň (žádné lupnutí). |

## 5. Grafika — stav

- Renderer **`gpu_naples2`** (`plugins/gpu_naples2/`) je nový, celý
  v **OpenGL ES 2**: kreslí primitiva shadery, texturování i palety (CLUT)
  počítá GPU, kreslí v souřadnicích VRAM 1:1.
- Starý plugin `gpu-gles` (OpenGL ES 1) byl **smazán** — nepřekládal se
  a jen mátl.
- Snímek se z GPU pořád čte přes `glReadPixels` (na vlákně emulace, takže
  už nebrzdí zvuk). **Přímá cesta přes sdílenou texturu zatím nefunguje** —
  sdílení kontextu projde a obraz má obsah, ale eglrender ji nevykreslí.
  Kód je v `egl_main.c` vypnutý přepínačem `POUZIT_PRIMOU_TEXTURU`.

## 6. Co zbývá (stav k B20)

- TV webviewer na `/8765` neukazuje obraz — používá snímání okna WebView
  (`PixelCopy`), které při běžícím emulátoru nemá co snímat
  (`TV_WEB_FRAME_ERR: Window doesn't have a backing surface`).
  **Snímky už jsou k dispozici** v `g_frame_argb` (plní je vlákno emulace),
  takže TV je stačí brát odtud.
- Dvě plátna (hra zvlášť, ovladač zvlášť).
- Chybí joystick a základní obrazovka.
- Emulátor skáče rovnou na šířku; má to dělat jen u širokoúhlého režimu.
- Přímá GPU cesta (bod 5).
