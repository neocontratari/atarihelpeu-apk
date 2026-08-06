# B84 — ČISTÉ JÁDRO, TENTOKRÁT SLINKOVANÉ (versionCode 132)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Proč B83 spadl

```
ld.lld: error: undefined symbol: n2_init
ld.lld: error: undefined symbol: n2_finish
```

Aplikace volá tyhle dvě funkce ze starého vykreslovače **přímo**
(`nap_ps1_native.cpp` řádky 733, 1209, 1280). Můstek je neměl.

**Proč jsem to nechytil:** kód jsem jen **překládal**, nikdy **nelinkoval**.
Chybějící symbol se při překladu neprojeví — ukáže se až při spojování
knihovny, tedy až u tebe v Actions. Ten log, co jsi poslal, to našel
okamžitě.

Opraveno: můstek obě funkce dodává.

## A přidal jsem si kontrolu, aby se to neopakovalo

Od teď u sebe dělám tohle:

1. přeložím `nap_ps1_native.cpp` na objekt pro ARM64
2. vypíšu si, které symboly **potřebuje** (`nm -u`)
3. vypíšu si, které jádro **dává** (`nm --defined-only`)
4. porovnám

U tohohle buildu vyšlo: **všech 27 symbolů sedí, knihovna se slinkuje.**
Je to zapsané v předávacím balíčku jako povinný krok.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, proti skutečnému `jni.h` | 0 chyb |
| jádro + gpu_neon | křížový překlad pro ARM64 | 0 chyb, ELF ARM aarch64 |
| **symboly** | `nm -u` proti `nm --defined-only` | **všech 27 sedí** |

## Obsah je jinak stejný jako B83

- překládá se **prověřený `gpu_neon`** (19 377 řádků, součást
  PCSX-ReARMed) místo ručně psaného (1 291 řádků)
- duch BIOSu v celé aplikaci: hlídač už neběží uvnitř smyčky pro TV,
  ale samostatně

---

## CO TESTOVAT

**1)** PS1 bez disku — menu BIOSu, na výšku i na šířku
**2)** Vrať se šipkou do **hlavní nabídky aplikace**
**3)** **Formule** — ta, co měla kostičky
**4)** **Star Wars** — ty, co měly artefakty
**5)** **Medal of Honor 2** — kontrola, že jsem nic nerozbil

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz jako dosud | horší než v B82 |
| 2 | **žádný obraz PS1** nikde v aplikaci | prosvítá bootovací Sony |
| 3 | **kostičky pryč** | pořád kostičkované |
| 4 | **artefakty pryč** | pořád artefakty |
| 5 | jako dosud, čistý zvuk | zhoršilo se |

## CO POSLAT ZPĚT

Očima podle tabulky. Z logu jeden řádek:

    VYKRESLOVAC: gpu_neon (provereny, soucast jadra PCSX-ReARMed)

A kdyby build zase zčervenal, ten log z Actions — posledně byl k nezaplacení.
