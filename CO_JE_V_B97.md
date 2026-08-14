# B97 — OPRAVY PÁDU + LOG, KTERÝ PÁD PŘEŽIJE (versionCode 145)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Měl jsi pravdu, ta věta byla blbost

Chtěl jsem po tobě log z `8765/log` po pádu — jenže ten se posílá **z běžící
aplikace**. Když spadne, není odkud. Opraveno:

**Log se teď píše i sem:**

    Downloads / AtariHelp / PS1_LOG.txt             <- aktualni relace
    Downloads / AtariHelp / PS1_LOG_predchozi.txt   <- relace PRED padem

Dostaneš se k nim z telefonu, bez počítače. Předchozí relace se **nemaže** —
takže i když aplikace spadne a ty ji zase spustíš, log z toho pádu tam
zůstane.

Při pádu se navíc zapisuje **přímo do souboru**, ne přes frontu, kterou
by pád nemusel přežít.

## Značky, ať je vidět, kam až se to dostalo

Do logu se teď zapisuje průběh zapínání TV:

    TV_PRIMO KROK1_EGL_SELHAL
    TV_PRIMO KROK2_KONFIGURACE_SELHALA
    TV_PRIMO KROK3_FORMAT_OKNA_OK fmt=...
    TV_PRIMO KROK4_KONTEXT_SELHAL (0x...)
    TV_PRIMO PRIPRAVENO: okno ...x...

A vlákno je napojené na odchytávač nativních pádů, který v projektu už byl —
takže po pádu bude v logu i `g_crash_stage`, tedy kde přesně to prasklo
(`TV: start vlakna`, `TV: priprava shaderu`, `TV: kreslici smycka`).

## Opravy pádu (z B96, ověřené znovu)

1. **Chyběl formát okna enkodéru** (`setBuffersGeometry`) — bez toho se
   vytvoření EGL plochy chová nepředvídatelně.
2. **Hrozilo zaseknutí na zámku** — Java volá odpojení z místa, kde drží
   zámek enkodéru, a nativní strana na své vlákno čekala. Teď nečeká.
3. **Dvě vlákna mohla kreslit naráz** — každé má číslo generace a jakmile
   přestane být aktuální, samo skončí.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** **Zapni TV** po startu PS1
**2)** Vypni a **zase zapni**, třikrát po sobě

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | aplikace běží dál, na TV obraz | spadne |
| 2 | přežije opakovaně | spadne napodruhé |

## CO POSLAT ZPĚT

**Když nespadne:** stačí to napsat.

**Když spadne:** otevři v telefonu **Downloads → AtariHelp** a pošli
`PS1_LOG_predchozi.txt`. Bude tam vidět, u kterého kroku to skončilo —
a to mi řekne příčinu bez hádání.
