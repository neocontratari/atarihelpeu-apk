# B171 — ČESKÁ KLÁVESNICE (versionCode 219)

## Ta pětka u uvozovek

Bral jsem **fyzickou klávesu** (`e.code`), tedy MÍSTO na klávesnici.
Jenže na české klávesnici je na tom místě něco jiného než na anglické —
proto se při psaní uvozovek objevila pětka.

Teď se bere **znak** (`e.key`), který už má rozložení započítané.
Fyzická klávesa se použije jen u toho, co znak nemá: Enter, šipky,
Backspace, Tab.

Ověřeno spuštěním proti skutečné tabulce kláves z Atari:

```
klávesa      znak    ->  výsledek
Digit2       "       ->  OK:Digit2=scan94      CZ: uvozovky
Digit5       "       ->  OK:Digit5=scan94      uvozovky jinde, stejný scan
KeyZ         z       ->  OK:KeyZ=scan23        Z na místě anglického Y
Semicolon    o       ->  OK:Semicolon=scan8    písmeno jinde
Digit8       (       ->  OK:Digit8=scan112     závorka
Enter        Enter   ->  OK:Enter=scan12       bere se klávesa
ArrowUp      ArrowUp ->  OK:ArrowUp=scan142    šipka

došlo do emulátoru: 11 z 11
```

Uvozovky dají `scan94` **ať leží kdekoli**.

## To vyskakování z aplikace — nenašel jsem to

Prošel jsem kód a **žádné `finish()`, žádný časovač, žádné zavření
aplikace tam není**. Ta jediná dvě místa, kde se otevírá prohlížeč,
se spustí jen při stahování souboru, ne při nečinnosti.

Nebudu ti tvrdit, že to mám. **Místo hádání jsem přidal hlášení**, které
zapíše každý odchod aplikace do pozadí i s tím, co v tu chvíli běželo:

```
BUILD2SA57 APLIKACE_PAUZA url=... tv=true ps1bios=false intro=false
           pametVolna=42MB pametMax=256MB
```

Až se to stane, **v logu bude vidět, co tomu předcházelo** — hlavně
jestli nedocházela paměť. To je nejčastější důvod, proč Android
aplikaci ukončí, a s běžícím emulátorem a TV castem je to možné.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| klávesy včetně CZ rozložení | 11/11 |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapoj USB klávesnici, otevři adresu v prohlížeči.
**3)** Napiš `10 PRINT "AHOJ"` — **projdou uvozovky správně?**
**4)** Nech aplikaci chvíli být a počkej, jestli vyskočí.

## CO POSLAT ZPĚT

Když to vyskočí, hledej v logu:

```
BUILD2SA57 APLIKACE_PAUZA ... pametVolna=..MB
```

Ten řádek mi řekne, co běželo a kolik zbývalo paměti — a podle toho
budu vědět, kde hledat.
