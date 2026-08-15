# B100 — OSTRÝ OBRAZ NA TV (versionCode 148)

> **Tento kód je předpoklad. Čeká se na test.**
> Sáhl jsem **jen na kvalitu obrazu pro TV**. Jádra, zvuku ani obrazu na
> mobilu jsem se nedotkl.

## Měl jsi pravdu se srovnáním

Když říkáš, že na originální PS1 na CRT je obraz čistý jako na mobilu a web
viewer má co dohánět — je to tak. Našel jsem dvě věci, které ho kazily.

## 1. Datový tok byl nízký

```java
KEY_BIT_RATE = w * h * 6      // u 1280x720 = 5,5 Mbit/s
```

Na obraz z PlayStation je to málo. Má **ostré pixely** a v pohybu detailní
textury (přesně jako NFS) — a H.264 při nízkém toku takový obraz rozmaže.

Teď je to `w * h * 20`, tedy **18 Mbit/s**. Přes wifi 5 GHz to projde
a enkodér to stíhá — v tvém logu je `avgDrawMs=2`, takže má rezervu.

## 2. Zvětšení obraz rozmazávalo

```java
Paint pp = new Paint(Paint.FILTER_BITMAP_FLAG);   // hladke zvetseni
```

Ten příznak **průměruje sousední body**. U fotky je to správně, u obrazu
z PlayStation ne — ten má ostré pixely a průměrováním se rozmaže.

A má to druhý efekt: rozmazaný obraz se **hůř komprimuje**, takže to ubíralo
i z toho datového toku.

Teď se každý bod jen zvětší — přesně jak to vypadá na skutečné konzoli.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Zapni **TV web** a pusť **NFS** — to, cos porovnával s konzolí
**2)** Nech chvíli běžet a sleduj plynulost

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz **ostřejší**, blíž k tomu na mobilu | stejný / horší |
| 2 | plynulost jako v B99 | začne se kousat |

Kdyby se to začalo kousat, je to tím vyšším datovým tokem přes wifi — řekni
a stáhnu ho na 12 Mbit/s. Procesor to nestojí nic, kóduje to hardware.

## CO POSLAT ZPĚT

Je obraz na TV ostřejší? A nekouše se?
