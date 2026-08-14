# B92 — TV BEZ BERLIČKY (versionCode 140)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Našel jsem tu berličku, o které mluvíš

Cesta na TV dělala tohle:

```java
for (int y = 0; y < sh; y++)
    for (int x = 0; x < sw; x++) {
        ... sahni na 4 sousedy, spocitej laplacian ...
    }
```

Je to **doostřování obrazu počítané v Javě bod po bodu**. Pro každý bod se
sáhne na čtyři sousedy — pět čtení na bod. U 400×300 to je **šest set tisíc
operací na každý snímek** a dělá to procesor, který pak chybí emulaci.

Vypnuto. Obraz už kreslí `gpu_neon` správně (PlayStation má ostré pixely,
doostřovat není co) a kvalitu na TV řeší H.264 enkodér.

Kdyby obraz na TV působil měkce, správná cesta je **zvýšit datový tok
enkodéru**, ne počítat filtr na procesoru. Zapsal jsem to do předávacího
balíčku.

## Čeho jsem se NEDOTKL

Schválně, ať to nerozhodíme:

- **ořez černých okrajů** — prochází jen sloupce, je to levné, zůstává
- **zvětšení na 1280×720** — dělá `Canvas.drawBitmap`, jde přes GPU, zůstává
- **H.264 enkodér** — hardwarový, zůstává
- **jádro, zvuk, obraz na mobilu** — nesahal jsem na ně vůbec

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Spusť PS1 a koukni na **TV**
**2)** Spusť **hru** a nech ji chvíli běžet, sleduj TV
**3)** Zkontroluj **mobil** — jestli je pořád v pořádku

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz na TV jako dosud | horší / rozmazaný |
| 2 | **plynulejší** než dosud, zvuk čistý | zhoršilo se |
| 3 | na mobilu **beze změny** | rozbité |

Obraz na TV může působit o chlup měkčeji — to doostřování tam bylo právě
proto. Když ti to bude vadit, zvýším datový tok enkodéru; to procesor nestojí.

## CO POSLAT ZPĚT

Jednou větou: je TV plynulejší a není obraz horší?
