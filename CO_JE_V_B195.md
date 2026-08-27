# B195 — CESTA OBRAZU ZPĚT NA B121 (versionCode 243)

M�l jsi pravdu a ten balíček to rozhodl. **B121 obraz na plátně jel** —
tak jsem se podíval, co jsem od té doby v té cestě změnil.

## Co jsem tam přidal a čím to rozbil

```
                        B121      B192-B194
zahazování snímků         ne          ANO      <- moje, rozbilo obraz
fronta pro diváka        300           24      <- moje
klíčový snímek po         2 s         1 s      <- moje
posílání snímků z Atari   ne          ANO      <- moje
```

**Všechno pryč.** Rozesílání snímků divákům je teď **shodné s B121** —
jedno `q.offer(chunk)` a nic víc.

Konkrétně to zahazování: v H.264 na sobě snímky navazují a **každý
vypadek je vidět až do dalšího klíčového**. Přidal jsem to v B192
s dobrým úmyslem a rozbil tím obraz. To, že jsem to v B194 „opravil"
vyžádáním klíčového snímku, byla další záplata na něčem, co tam vůbec
nemělo být.

## A vyhodil jsem po sobě 5650 znaků mrtvého kódu

Zbytky po tom posílání snímků z Atari — cesta na serveru, funkce,
proměnné. Atari se snímá z okna jako v B121.

## Cesta obrazu proti B121

```
rozesílání snímků    shodné
fronta               300 = 300
klíčový snímek       2 s = 2 s
snímků za vteřinu    60 = 60
napTvWebPublishBitmap  shodná
napTvWebCaptureFromCore  shodná až na 3 řádky pro intro
```

Ty tři řádky jsou jediné, co v té funkci proti B121 přibylo, a jsou tam
kvůli obrazu jader v intru.

## Co zůstává funkční

```
intro           etapa1 -> etapa2 -> Sega -> PS1 -> etapa5 -> konec
klávesnice      Atari 9 z 9,  Sega 8 z 8
stránka pro TV  spuštěna v node, 0 chyb
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| stránka pro TV SPUŠTĚNA | 0 chyb |
| řetěz intra | projde celý |
| klávesnice Atari / Sega | 9/9 a 8/8 |
| cesta obrazu proti B121 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Plátno — jede jako v B121? Musíš dávat refresh?**

To je ta jediná otázka, na které teď záleží.
