# B137 — STAHOVÁNÍ SE SOUHLASEM (versionCode 185)

> Stavěno na B136, tedy na čistém B131. **Nic z toho, co fungovalo, se nezměnilo.**

## Co je nového

Při **prvním spuštění** se aplikace zeptá:

```
Stahnout ze stranek atarihelp.eu?

  • Sonic the Hedgehog  (asi 0,5 MB)
  • BIOS pro PlayStation  (asi 1 MB)

Ulozi se do slozky Download/AtariHelp a zustanou v telefonu -
budes je moci pouzit i bez internetu.

Bez nich aplikace funguje dal, jen se v uvodnim filmu preskoci
cast se Segou a PlayStation.

              [STAHNOUT]     [TED NE]
```

**Aplikace si nic nestahuje sama od sebe.** Dialog řekne co, odkud a kolik,
a stahuje se až po odklepnutí.

```
Sonic  ->  Download/AtariHelp/emu/sega
BIOS   ->  Download/AtariHelp/emu/ps1
```

Kdo řekne **TEĎ NE**, najde v nabídce OPTIONS nové tlačítko
**STAHNOUT HRY A BIOS** a může si to pustit kdykoli později.

## Proč to takhle

Ty soubory jsou z tvého webu a jsou tvoje. Když je má uživatel na telefonu,
může dvě věci:

1. **hrát i bez internetu**
2. intro může na dvě tři vteřiny **spustit skutečné jádro** Segy i PS1 —
   obrazovka a zvuk pak **vzniknou výpočtem** z jeho vlastního souboru,
   úplně stejně jako když si hru pustí. Nic se z aplikace nepřehrává.

Když se nestáhne nic, intro tu část prostě přeskočí a nic se nerozbije.

## Ověřeno u mě

Rozbalování jsem **pustil na tvém skutečném souboru**, ne jen zkontroloval:

```
ZIP:      384 081 B
ven leze: Sonic The Hedgehog (USA, Europe).gen   524 288 B
vysledek: sonic=OK(1)
```

Tlačítko v OPTIONS jsem **spustil** proti napodobenině prohlížeče — volá
do Javy `stahniSoubory()`.

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 44 tříd |
| JS rozcestníku | 0 chyb |
| rozbalení tvého ZIPu | ROM 524 288 B, sedí |
| tlačítko v OPTIONS | volá do Javy |
| Atari, PS1, Sega | **nesaháno** |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nainstaluj a spusť — **objeví se ta otázka?**
**3)** Dej **STAHNOUT**. Zkontroluj `Download/AtariHelp/emu/sega`
       a `Download/AtariHelp/emu/ps1`.
**4)** Spusť aplikaci znovu — **otázka už se ptát nemá**.
**5)** V nabídce OPTIONS zkus **STAHNOUT HRY A BIOS**.
**6)** Atari, PS1, Sega, WEB TV — jako v B131.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | otázka se objeví | nic |
| 3 | v obou složkách soubory | prázdno |
| 4 | neptá se znovu | ptá se pořád |
| 5 | zeptá se a stáhne | nic |
| 6 | jako v B131 | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej v něm:

```
BUILD2SA27 STAZENI sonic=OK(1) bios=OK(1)
```

Když tam bude něco jiného než `OK`, řekne to přesně proč — `HTTP404`,
`nic-v-zipu`, název výjimky.

---

## Co bude dál

Až tohle projde, přijde druhý krok: **intro spustí jádro Segy s tou ROM**,
takže naskočí skutečná obrazovka i zvuk, počítané z YM2612 a PSG té hry.
A to samé s PS1 BIOSem.

Dělám to po krocích schválně — ať se dá u každého poznat, jestli sedí.
