# B164 — BEZ PIXELCOPY (versionCode 212)

## Trefil jsi příčinu

Napsal jsi: *„jinak posíláš obraz a zvuk z Atari než ze Segy a PS1."*
Ověřil jsem to v kódu a je to přesně tak:

```
SEGA a PS1   snímek jde PŘÍMO Z JÁDRA  ->  okno se nesnímá vůbec
ATARI        PixelCopy CELÉHO OKNA
```

`PixelCopy` si vynutí **čtení zpátky z grafické karty**. Tím se zastaví
celý vykreslovací řetěz — a v tom řetězu kreslí i to plátno, na kterém
běží Atari.

**Proto se kouše hned po zapnutí TV, ať tempo osekám jakkoli.** Ani
zpomalení, ani menší rozlišení, ani zrychlení mostu s tím nepohnou —
problém není v tom, KOLIK to stojí, ale v tom, ŽE to zastaví grafiku.

## Co s tím jde udělat hned

V kódu už je druhá cesta: `napTvWebCaptureByDraw()` kreslí **procesorem**
a grafiku nezastaví. Atari ji ale nedostane, protože je v seznamu
obrazovek, kde se jede přes `PixelCopy`.

Tlačítko `TV POKUS` má proto novou čtvrtou volbu:

```
TV POKUS: OBOJI       normální provoz
TV POKUS: JEN ZVUK    obraz se nesnímá
TV POKUS: JEN OBRAZ   zvuk se neposílá
TV POKUS: KRESLENIM   Atari bez PixelCopy   ← tohle vyzkoušej
```

## Co potřebuju

Zapni TV, jdi do Atari, přepni na **KRESLENIM** a zahraj si.

| co se stane | co to znamená |
|---|---|
| **kousání zmizí a obraz na TV jde** | máme to, udělám z toho natvrdo |
| kousání zmizí, ale **TV je černá** | `draw()` neumí vytáhnout obsah plátna — půjdu jinou cestou |
| kouše se dál | mýlím se i tady a řeknu ti to rovnou |

To druhé je docela pravděpodobné — `draw()` u plátna s grafickou
akcelerací často vrátí prázdno. Proto to dávám jako **pokus, ne jako
opravu.**

## Kdyby to nevyšlo

Pak zbývá jediná poctivá cesta: **Atari by muselo dávat snímek samo**,
jako to dělá Sega a PS1 — tedy předávat obsah plátna přímo do Javy.
To je zásah do `emu_vbxe` a na to potřebuju tvoje svolení.

Bez toho už si nepomůžu a nebudu předstírat opak.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS rozcestníku i etap | 0 chyb |
| Java Atari (`emu_vbxe`) | netknuté proti B123 |
| jádra Segy i PS1 (C++) | nesaháno |
