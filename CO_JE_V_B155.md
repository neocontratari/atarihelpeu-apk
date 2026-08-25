# B155 — INTRO NA TV (versionCode 203)

Dvě příčiny, obě moje.

## 1) Na TV nebyl obraz z etap 1 a 5

Okno jsem dělal **průhledné po celou dobu intra**, aby byla vidět plocha
jádra pod ním. Jenže TV snímá okno přes `PixelCopy` — a z průhledného
okna sejme prázdno.

Teď, když jsou etapy samostatné stránky, stačí průhlednost zapnout
**jen po dobu živých jader** a hned po nich vrátit. Etapy 1 a 5 mají okno
neprůsvitné, takže je TV sejme.

## 2) Na TV nebyl zvuk z etap

Zvuk si intro dělalo samo v prohlížeči (Web Audio). Na telefonu hrál,
ale **na TV ne** — tam se zvuk bere přes `napTvWebAudioPush()` a do
WebView se nedosáhne.

Teď se vzorky **počítají v Javě** a posílají se na obě strany zároveň:

```
AudioTrack          -> reproduktor telefonu
napTvWebAudioPush   -> WEB TV
```

Přesně tou cestou, kterou už používá Atari, Sega i PS1.

Ověřeno spuštěním té syntézy — 60 vteřin hudby a klapání:

```
vzorků 5 324 800, nenulových 84,7 %, průměr 1543, špička 19 082 z 32 000
```

Spouštění zvuku z prohlížeče jsem odstranil, aby to nehrálo dvakrát —
ověřeno, ve všech třech etapách 0 výskytů.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 49 tříd |
| JS všech tří etap | 0 chyb |
| syntéza v Javě vyrábí vzorky | 84,7 % nenulových |
| řetěz etap | 1 → 2 → jádra → 5 → konec |
| jádra Segy i PS1 (C++) | nesaháno |

---

## K té otázce o instalaci z netu

**Jde to.** Android sice instaluje z souboru, ale přes `PackageInstaller`
se dá APK **streamovat rovnou ze sítě do instalační relace** — soubor
pak nikde na disku neleží a v Souborech se neobjeví. Uživatel jen odklepne
systémové okno „Nainstalovat".

Potřebuje to oprávnění `REQUEST_INSTALL_PACKAGES`. Když to budeš chtít,
udělám to jako samostatný krok.

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV a pusť intro.
**3)** Etapa 1 a 2 — **jde na TV obraz i zvuk?**
**4)** Sega a PS1 — obraz i zvuk?
**5)** Etapa 5 s krtečkem — obraz i zvuk?
**6)** Na mobilu má být všechno jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | obraz i zvuk | jen jedno nebo nic |
| 4 | obraz i zvuk | jen zvuk |
| 5 | obraz i zvuk | prázdno |
| 6 | jako dosud | zvuk hraje dvakrát |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA41 INTRO_ZACATEK okno neprusvitne
BUILD2SA41 INTRO_ZACATEK zvuk v Jave
```
