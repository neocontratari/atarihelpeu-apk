# B143 — BIOS NA SPRÁVNÉ MÍSTO (versionCode 191)

## Chyba byla v mém stahovači

```
moje stahování ukládalo BIOS do:   Download/AtariHelp/emu/ps1
PS1 ho ale hledá v:                Download/AtariHelp
                                   Download/AtariHelp/BIOS
                                   Download/AtariHelp/PS1_BIOS
```

Do `emu/ps1` se PS1 nikdy nepodívá. Položil jsem BIOS na místo, které
nikdo nečte — a PS1 si ho proto stahovalo znovu ze sítě.

**Ověřeno spuštěním** tou samou logikou, jakou má `ps1EnsureBios()`:

```
uloženo do emu/ps1     -> PS1 ho najde: NE - stáhne si vlastní znovu
uloženo do PS1_BIOS    -> PS1 ho najde: ANO
```

Nově se ukládá do `Download/AtariHelp/PS1_BIOS`. Kontrola „mám BIOS?"
prohledává stejné složky jako PS1, plus starou cestu z B137, aby se
nestahoval znovu tomu, kdo už ho tam má.

## Zásah do cyklu PS1 je vrácen

Řekl jsi, že na cyklus PS1 sahat nemám. Vráceno —
`stopPs1SessionHard("intro...")` se v kódu vyskytuje **nulakrát**.
Intro jen požádá o start BIOSu; když už běží, nic se neděje.

## Zůstává z B142

**Původní grafika Segy a PS1.** Během živých jader se zprůhlední tělo
stránky, rastr CRT i plátno — černé pozadí stránky předtím překrývalo
obraz, který jádro kreslilo pod ní.

**Zvuk PS1 na TV.** Po části se Segou se nuluje značka „jiný zdroj právě
posílá", která PS1 na TV umlčovala.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS intra i rozcestníku | 0 chyb |
| PS1 najde BIOS na novém místě | ověřeno spuštěním |
| pořadí volání do Javy | ověřeno spuštěním |
| jádra Segy i PS1 (C++) | nesaháno |
| cyklus PS1 v Javě | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Smaž `Download/AtariHelp`** celou a spusť aplikaci.
**3)** Dej STAHNOUT. Zkontroluj, že BIOS je v `Download/AtariHelp/PS1_BIOS`.
**4)** Nech film běžet — vidíš **původní obrazovku Segy** a po ní **PS1**?
**5)** Pusť intro znovu z OPTIONS.
**6)** Zapni WEB TV a pusť intro — jde zvuk PS1 na TV?
**7)** Atari, PS1, Sega — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | BIOS v PS1_BIOS | jinde nebo nikde |
| 4 | obě původní obrazovky | černo nebo moje grafika |
| 6 | zvuk PS1 na TV | ticho |
| 7 | jako dosud | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA27 STAZENI sonic=OK(1) bios=OK(1)
BUILD2SA7 PS1_BIOS_ADOPTED count=...
```

Ten druhý řádek znamená, že PS1 BIOS **našlo** a nestahovalo si ho znovu.
Když místo něj uvidíš `PS1_BIOS_AUTO_DOWNLOADED`, pořád si ho stahuje
a mám hledat dál.
