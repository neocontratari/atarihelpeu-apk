# B139 — KONTROLA SOUBORŮ (versionCode 187)

Opravuje to, cos našel: aplikace si jen pamatovala, **že už se ptala**,
ale nekontrolovala, **jestli ty soubory pořád jsou**. Když jsi složku
`Download/AtariHelp/emu` smazal, tvrdohlavě mlčela.

## Teď rozhoduje disk, ne vzpomínka

```
Sonic   hledá .gen / .md / .smd / .bin od 32 kB výš
        v Download/AtariHelp/emu/sega a přímo v AtariHelp

BIOS    hledá .bin o velikosti přesně 524 288 B
        v emu/ps1, PS1_BIOS, BIOS a přímo v AtariHelp
```

**Chybí-li cokoli z toho, aplikace se zeptá znovu** — i když už se jednou
ptala. Když je všechno na místě, mlčí a rovnou pustí film.

## Ověřeno spuštěním, ne odhadem

Projel jsem tou samou logikou pět stavů složky:

```
nic tam není         sonic=CHYBI  bios=CHYBI     PTÁM SE
jen Sonic            sonic=je     bios=CHYBI     PTÁM SE
jen BIOS             sonic=CHYBI  bios=je        PTÁM SE
obojí                sonic=je     bios=je        neptám se
složka emu smazána   sonic=CHYBI  bios=CHYBI     PTÁM SE
```

Do logu se navíc zapíše, co našel:

```
BUILD2SA29 SOUBORY sonic=Sonic The Hedgehog (USA, Europe).gen(524288B) bios=je -> vse je, neptam se
```

## Ověřeno u mě

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS rozcestníku, intra i Segy | 0 chyb |
| rozpoznání pěti stavů složky | 5/5 správně |
| Atari, PS1, Sega | **nesaháno** |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci — když už soubory máš, **nemá se ptát**.
**3)** **Smaž složku `Download/AtariHelp/emu`** a spusť znovu —
       **teď se zeptat MÁ**.
**4)** Smaž jen Sonica a nech BIOS — má se zeptat taky.
**5)** Atari, PS1, Sega, WEB TV — jako v B131.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | mlčí a pustí film | ptá se zbytečně |
| 3 | zeptá se znovu | mlčí |
| 4 | zeptá se znovu | mlčí |
| 5 | jako v B131 | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA29 SOUBORY sonic=... bios=... -> PTAM SE / vse je, neptam se
```

Ten řádek řekne přesně, co aplikace na disku našla a jak se rozhodla.

---

## Co bude dál

Až tohle projde, přijde ten krok, kvůli kterému to celé bylo:
**intro spustí jádro Segy s tou ROM** — naskočí skutečná obrazovka i zvuk,
počítané z YM2612 a PSG té hry. A to samé s PS1 BIOSem.
