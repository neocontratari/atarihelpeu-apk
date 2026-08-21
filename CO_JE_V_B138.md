# B138 — OTÁZKA PŘED INTREM (versionCode 186)

Opravuje čtyři věci, co jsi našel v B137. Všechny sedí a všechny byly moje.

## 1) Otázka teď přijde PŘED filmem

V B137 se dialog objevil **přes už běžící intro** — film hrál pod ním,
doběhl a skočil do menu, aniž jsi stihl cokoli odklepnout.

Teď je pořadí správně:

```
1. prázdná obrazovka        pod dialogem nic neběží a nic neutíká
2. otázka na stažení
3. stahování s průběhem
4. výsledek + POKRACOVAT
5. AŽ TEĎ se pustí film
```

## 2) Stahování je vidět

```
Stahuji
Sonic the Hedgehog
[████████░░░░░░░░]
312 kB z 375 kB   840 kB/s
```

Název souboru, kolik už je, kolik celkem a jakou rychlostí. Hlásí se
pětkrát za vteřinu.

## 3) Na konci se potvrdí, co se stáhlo

```
Stazeno

✓ Sonic the Hedgehog - ulozen
✓ BIOS pro PlayStation - ulozen

Slozka: Download/AtariHelp/emu

              [POKRACOVAT]
```

Když se něco nestáhne, napíše se to a přidá se, že to jde zkusit znovu
v nabídce OPTIONS — i s technickou příčinou.

## 4) Pojistka proti prázdné obrazovce

Kdyby dialog z jakéhokoli důvodu nenaskočil, zůstal bys na prázdné
obrazovce. Proto:

- film se smí spustit **jen jednou**, ale **musí se spustit vždycky**
- všech pět cest (stáhnout, teď ne, zrušit, chyba dialogu, chyba stahování)
  vede k jeho spuštění — ověřeno v kódu, ne odhadem
- k tomu časová pojistka: kdyby se do dvou minut nic nestalo, film se
  pustí sám a do logu se zapíše proč

## Ověřeno u mě

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS rozcestníku, intra i Segy | 0 chyb |
| rozbalení tvého ZIPu | ROM 524 288 B, sedí |
| všechny cesty vedou ke spuštění filmu | ověřeno v kódu |
| Atari, PS1, Sega | **nesaháno** |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Odinstaluj a nainstaluj znovu** — jinak si aplikace pamatuje,
       že už se ptala, a otázka nepřijde.
**3)** Po startu má být **prázdná obrazovka a hned otázka** — žádný film pod ní.
**4)** Dej **STAHNOUT** — vidíš průběh a rychlost?
**5)** Po dokončení dej **POKRACOVAT** — teprve teď má naskočit film.
**6)** Zkontroluj `Download/AtariHelp/emu/sega` a `/ps1`.
**7)** Spusť aplikaci znovu — otázka už se ptát nemá, film naskočí rovnou.
**8)** Atari, PS1, Sega, WEB TV — jako v B131.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | prázdno + otázka | film hraje pod dialogem |
| 4 | průběh a rychlost | jen "čekejte" nebo nic |
| 5 | film až po odklepnutí | film jel dřív |
| 6 | v obou složkách soubory | prázdno |
| 7 | neptá se, film naskočí | ptá se pořád |
| 8 | jako v B131 | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA28 START ptam-se-na-stazeni
BUILD2SA27 STAZENI sonic=OK(1) bios=OK(1)
BUILD2SA28 START url=... intro=SPOUSTIM
```

Ty tři řádky musí být **v tomhle pořadí**. Kdyby `START url=` přišel dřív
než `STAZENI`, pořadí je pořád špatně.
