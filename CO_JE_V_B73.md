# B73 — OBRAZ NA ŠÍŘKU A BEZ KOUSÁNÍ (versionCode 121)

Grafika BIOSu z B72 zůstává opravená. Tohle řeší ty dvě věci, cos popsal.

## 1. Kousání obrazu i zvuku ve hře

Z tvého logu:

    PS1_PREVIEW_AVG avgMs=77 srcW=512 srcH=240

**Sedmdesát sedm milisekund na snímek** jen na přípravu obrazu pro stránku.
Důvod byl v kódu:

```java
int sw = w * 3, sh = h * 3;                          // zvetseni 3x
...compress(CompressFormat.JPEG, 90, bo);            // a kvalita 90
```

Obraz 512×240 se zvětšoval na 1536×720 a teprve pak balil. Zvětšovat ho
nemá smysl — stránka si ho roztáhne sama přes CSS. Teď se posílá v původní
velikosti a v kvalitě 80.

Zátěž rendereru byla přitom celou dobu v pořádku (0,5 nahrání videopaměti
na snímek), takže tohle je ta jediná příčina kousání.

## 2. Černá obrazovka s D-padem na šířku

Ve stránce bylo tohle:

```js
if(window.innerWidth>window.innerHeight){
    im.style.display='none';
    return;                    // na sirku se obraz VUBEC neposilal
}
```

V komentáři nad tím stálo, že na šířku prý kreslí nativní `TextureView`.
**Ta třída ale byla mrtvá** (ležela za `if (true) return;`) a nakonec
smazaná — takže na šířku nekreslil nikdo. Odtud černá obrazovka jen
s ovladačem.

Obraz se teď posílá v obou otočeních. Ovladač zůstává nad ním: obraz má
z-index 2, tlačítka 12 a 16.

## Na TV se ovladač neobjeví

TV si bere snímek přímo z jádra, ne z obrazovky telefonu. Beze změny.

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |

## Co testovat

1. **Hra na šířku na mobilu** — má být obraz a nad ním ovladač.
2. **Plynulost** ve hře přes ISO CD i LOAD GAME.
3. V logu: `PS1_PREVIEW_AVG avgMs=` má spadnout ze 77 na jednotky.

## Nově: předávací balíček v každém buildu

V balíčku je `PREDAVACI_BALICEK_PS1.txt`. Od teď bude v **každém**.
Popisuje krok za krokem, s čísly řádků v kódu:

- kudy teče **obraz** — od jádra přes OpenGL ES až na displej a na TV,
  a přesně kde opouští GPU
- kudy teče **zvuk** — od jádra přes kruhovou frontu po OpenSL ES
- čím se co u mě překládá a ověřuje bez telefonu
- **slepé uličky**, které už jsou vyzkoušené a změřené, ať je nikdo neopakuje
- jak s tebou pracovat, ať to další chat nemusí objevovat znovu
