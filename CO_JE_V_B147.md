# B147 — PS1 SE OPRAVDU ZASTAVÍ (versionCode 195)

## Jedna příčina, tři tvoje příznaky

```java
boolean hadSession = ps1BootActive || ps1SessionActive || ps1CurrentAudioTrack != null ...
if (!hadSession && !hadRemoteDownload) return "PS1_ALREADY_STOPPED";
```

**Chybělo tam `ps1BiosRunning`.** Když běžel jen BIOS bez disku, podmínka
byla nepravdivá a `stopPs1SessionHard()` se **hned vrátila, aniž by jádro
zastavila**.

V logu je to vidět tím, že tam **není ani jedno `PS1_SESSION_STOP`** —
přestože se ta funkce volala.

A protože se vrátila hned, neproběhl ani úklid, který je uvnitř ní:

```java
stopPs1Audio();              ← PS1 dál cpalo zvuk na TV
ps1DeactivateNativeView();   ← obraz PS1 zůstal přes zbytek filmu
```

Z toho plyne všechno, cos našel:

| co jsi viděl | proč |
|---|---|
| podruhé Memory Card místo znělky | `bootBiosSafe()` běžel na pořád běžícím jádře a nic nezresetoval |
| po PS1 nebylo vidět Atari s krtečkem | `ps1DeactivateNativeView()` neproběhlo, obraz PS1 zůstal navrchu |
| na TV nešel zvuk Atari po PS1 | `stopPs1Audio()` neproběhlo, PS1 drželo zvukovou cestu |

## Oprava

Do podmínky se doplnilo `ps1BiosRunning` — na dvou místech:
`stopPs1SessionHard()` a `stopPs1IfLeaving()`.

Ověřeno spuštěním:

```
stav                stará (B146)   nová (B147)
hraje se hra        zastaví        zastaví
bootuje se hra      zastaví        zastaví
běží JEN BIOS       NEZASTAVÍ      zastaví
nic neběží          nezastaví      nezastaví
```

> Tohle je ta chyba, kterou mám v protokolu zapsanou jako **Bug #1** od
> první analýzy PS1 — čekala na tvoje svolení, protože sahá do PS1.
> Tady se projevila naplno, tak ji opravuju.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS intra i rozcestníku | 0 chyb |
| rozhodnutí podmínky ve 4 stavech | 4/4 správně |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nech film běžet celý — **po PS1 se musí objevit krteček**.
**3)** Pusť intro **podruhé** — musí zaznít znělka Sony, ne Memory Card.
**4)** Zapni WEB TV, nech film běžet — **jde zvuk i obraz u všech tří částí?**
**5)** Zahraj si hru na PS1, vrať se do menu — **musí se korektně zastavit.**

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | krteček po PS1 | zůstane obraz PS1 |
| 3 | znělka Sony | Memory Card / Play CD |
| 4 | zvuk i obraz všude | něco chybí |
| 5 | zastaví se | běží dál nebo pád |

**Krok 5 je ten, kde buď přísný.** Sáhl jsem na podmínku, která rozhoduje,
kdy se PS1 zastavuje — dotýká se to i normálního hraní.

## CO POSLAT ZPĚT

Log. Teď už v něm **musí být**:

```
BUILD2SA5I PS1_SESSION_STOP reason=intro:...
```

Když tam nebude, jádro se pořád nezastavuje.
