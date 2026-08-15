# B101 — KOSTKY NA TV PRYČ (versionCode 149)

> **Tento kód je předpoklad. Čeká se na test.**
> Sáhl jsem **jen na nastavení enkodéru pro TV**. Nic jiného.

## Ten screenshot ukázal příčinu

Kostky v **tmavých plochách** — na silnici a v pozadí. To není nedostatek
datového toku, to je tohle:

```java
KEY_I_FRAME_INTERVAL = 1     // klicovy snimek KAZDOU VTERINU
```

Klíčový snímek je asi **desetkrát dražší** než běžný. Při pevném datovém toku
si vezme velkou část rozpočtu — a na snímky kolem něj **nezbude**, takže se
rozpadnou do kostek. Nejvíc je to vidět v tmavých plochách, kde nemá H.264
detaily, do kterých by chybu schoval.

## Tři opravy

**1. Klíčový snímek každé 2 vteřiny** místo každou. Rozpočet se rozloží
rovnoměrněji.

**2. Proměnný datový tok (VBR).** Ve výchozím stavu enkodér drží tok
konstantní — do klidné scény cpe bity zbytečně a na složitou už je nemá.
S VBR jdou bity tam, kde jsou potřeba.

**3. Tok z 18 na 27 Mbit/s.** Enkodér má rezervu — v tvém logu `avgDrawMs=3
až 4` při stropu 16 ms. A říkáš, že mobil i wifi na to mají.

## K té otázce, jestli jedeme přes GPU

Prošel jsem tu cestu ještě jednou:

    jadro -> snimek v plne kvalite (ARGB 8888)
          -> jednim krokem na 1280x720 pres Canvas (GPU)
          -> H.264 enkoder (hardware)

**Mezi jádrem a enkodérem se kvalita neztrácí.** Snímek jde v plné kvalitě
a zvětšuje se jednou, ne dvakrát. Takže co ještě šlo zlepšit, bylo právě
v nastavení enkodéru — a to jsou ty tři věci výše.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Zapni TV a pusť **NFS** — to intro s auty v noci
**2)** Sleduj hlavně **tmavé plochy** (silnice, pozadí)

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | plynulost jako v B100 | začne se kousat |
| 2 | **kostky v tmavých plochách pryč nebo výrazně míň** | stejné |

Kdyby se to kouslo, je to tím vyšším tokem — řekni a stáhnu ho zpátky na
18 Mbit/s. Procesor to nestojí nic, kóduje hardware.

## CO POSLAT ZPĚT

Jsou kostky menší? A nekouše se?
