# B77 — OTOČENÍ A OKÉNKO (versionCode 125)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**
> Ověřeno: překlad Javy proti `android.jar` a překlad C++ přes clang
> pro aarch64. Chování na Mali ověřené není.

## 1. Obraz vzhůru nohama

Moje chyba. Vzal jsem z ověřené cesty smyčku, ale souřadnice textury jsem
napsal po svém a prohodil je. Správně je to takhle a je to teď doslova
opsané z `eglrender`:

```c
-1,-1, 0,0    1,-1, 1,0
-1, 1, 0,1    1, 1, 1,1
```

Y v OpenGL je zdola a pixely snímku jsou taky zdola, takže se to srovná samo.

Na TV byl obraz správně proto, že TV jde úplně jinou cestou — bere snímek
z jádra a kreslí ho sama.

## 2. Na výšku nebyl obraz v okénku konzole

Plocha ležela **pod** stránkou a přes celou obrazovku. Jenže grafika konzole
`ps1_final_screen.png` je **úplně neprůhledná** (změřeno: průhlednost 255
i v místě okénka), takže obraz zakryla. Na šířku je stránka průhledná, proto
tam obraz byl — jen vzhůru nohama.

Teď stránka hlásí, **kde přesně má obraz být**, a plocha se tam postaví:

- **na výšku** → plocha je NAD stránkou, přesně v okénku konzole
- **na šířku** → plocha je POD stránkou, přes celou obrazovku, ovladač nad ní

## Co zůstává na příště — ISO CD

V logu je vidět rozdíl, proč se u ISO CD kouše zvuk a u LOAD GAME ne:

    ISO CD    -> cesta=/proc/self/fd/146
    LOAD GAME -> cesta=/data/.../ps1/Crash_Bandicoot.cue

ISO CD předává hru jako **popisovač souboru**, ne jako skutečný soubor.
Jádro s ním musí při čtení disku posouvat a to je nejspíš pomalé. Jeden
pokus dokonce skončil `PS1_HRA_FAIL jadro`.

Nesahám na to v tomhle buildu, ať se to nemíchá s opravou zobrazení.

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |

## CO TESTOVAT

**1) PS1 bez disku, telefon na VÝŠKU**
Spusť PS1 a nech naskočit menu BIOSu.

**2) Otoč telefon na ŠÍŘKU** (pořád v BIOSu)

**3) LOAD GAME z netu** — nech naběhnout hru, na výšku i na šířku

**4) TV prohlížeč** — koukni se na něj u kroků 1 až 3

---

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz **v okénku konzole**, bubliny nahoře, nápisy čitelně | okénko černé / obraz mimo okénko / vzhůru nohama |
| 2 | obraz **přes celou obrazovku**, správně otočený, ovladač **nad** ním a funkční | vzhůru nohama / obraz chybí / ovladač zmizel |
| 3 | totéž co 1 a 2, plynulý obraz i zvuk | kousání |
| 4 | obraz roztažený, **bez ovladače** | ovladač na TV / neroztažené / černo |

Na **ISO CD** teď nesahej — vím o tom, že se u něj kouše zvuk, a je to
zapsané jako další krok. Nepleťme to do jednoho testu.

---

## CO POSLAT ZPĚT

Očima: u kroků 1 až 4 jen **správně / špatně** podle tabulky výše.

Z logu tyhle řádky:

    PS1_OBRAZ_PRIMO_ZAPNUT
    PLOCHA_VYTVORENA
    PLOCHA PRIPRAVENA
    PLOCHA_UMISTENA          <- rekne mi, kam se plocha postavila
    PLOCHA: snimku=...       <- rekne mi, jestli kresli a z jakeho zdroje
    PS1_OBRAZ_MISTO          <- co hlasi stranka

Kdyby byl obraz černý, hledej `PLOCHA: kontext se nepodarilo pripravit`
— to bude i s číslem chyby a poznám z toho příčinu bez hádání.
