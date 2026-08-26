# B176 — CESTA PRO PS1 A SEGU ZPĚT (versionCode 224)

Ta tvoje práce navíc se vyplatila. **B156 na projektoru běží** — a to mi
řeklo přesně, kde hledat.

## Co jsem provedl

Funkce, která dodává snímek pro PS1 a Segu, měla v B156 **256 řádků**.
Já do ní přidal Atari, intro a diagnostiku, která se počítala u **každého
snímku** — a narostla na **309 řádků**. Běželo to i když jsi hrál PS1.

```java
// B156:
boolean naPs1 = (u.contains("emu_ps1") || u.contains("emu_sega"));

// co jsem tam přidal:
boolean naPs1 = (... || u.contains("emu_vbxe"));   // Atari
if (!naPs1 && !introZivaCast) { ... }              // intro
int wh = jeAtariTv ? atariFbVyzvedni(...)          // větev navíc
if (introZivaCast) { ...diagnostika každý snímek... }
```

Na Chromu se to ztratí. **Na tom projektoru ne.**

Sáhl jsem do cesty, kterou jsi označil za zavřenou — a přesně tam to
rozbil.

## Co je teď

**Funkce je vrácená bajt po bajtu do stavu B156.** Ověřeno:

```
B156:  256 řádků
teď:   256 řádků
shodná bajt po bajtu:      ANO
zmínky o Atari uvnitř:     0
```

Atari má **vlastní cestu vedle** — zkusí se první, a když nemá nový
snímek, pustí to dál beze změny.

Tempo snímání je taky zpátky na B156. Jediná úprava: **živá jádra
v intru** používají rychlou větev, jinak by intro nešlo.

## Co ti zůstává

```
intro s pěti etapami, písmo z tvé ROM
Turbo BASIC s polovičním strojakem, ověřený v Altirře
klávesnice z počítače, zapne se sama v Atari
Sonic a BIOS se stahují se souhlasem
postup hráče a zadní vrátka
aktualizace ze sítě
oprava té mé boty s kopírováním snímku
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| cesta PS1 a Segy proti B156 | shodná bajt po bajtu |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Na projektoru **PS1 a Sega** — jedou aspoň jako v B156?
**3)** Atari — jde obraz a píše klávesnice?

Krok 2 je ten hlavní.

---

## To kousání v PS1

Píšeš, že se PS1 krapet kouše i v B156 — takže to je starší věc, ne moje.
Až budeš mít tenhle build ověřený, můžu se na to podívat **s čistým
základem**, kde vím, že mezi tím nic mého nepřekáží.
