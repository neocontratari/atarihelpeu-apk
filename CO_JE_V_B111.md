# B111 — VÝŘEZ JEN PŘI HŘE (versionCode 159)

> **Tento kód je předpoklad. Čeká se na test.**

## Ten výřez — čtvrtá stížnost, teď konečně příčina

Moje podmínka se ptala **jen na to, jestli jsi na stránce PS1**. Jenže ta
stránka je i **úvodní obrazovka s konzolemi**, kde žádná hra neběží — a tam
se ořízlo prázdné okénko. Odtud ten nesmyslný výřez místo celé obrazovky.

Teď se ořezává **jen když jádro opravdu kreslí** (běží BIOS nebo hra).
Na úvodní obrazovce, v Atari i v Sega jde na televizi celé okno, jak jsi
chtěl.

Omlouvám se, že to trvalo čtyři kola — hlásil jsi to jasně, jen jsem
opravoval špatné místo.

## A to zjemnění, cos popsal

Píšeš, že se zapnutým vyladěním je míň kostiček, ale míň ostré, a že se to
dá dorovnat kontrastem. Máš pravdu — tak jsem ten kontrast nastavil jako
**výchozí**:

    kontrast  112 %   (bylo 100)
    sytost    108 %   (bylo 100)

Počítá to grafika prohlížeče, takže telefon to nestojí nic. Když ti to bude
moc, stáhni posuvník; **PUVODNI NASTAVENI** teď vrací tyhle hodnoty, ne
neutrální.

A našel jsem u toho ještě jednu chybu: filtr se používal **jen když už byly
uložené hodnoty** — takže při prvním spuštění se neaplikoval vůbec. Opraveno,
ověřeno spuštěním:

    1) SPUSTENI -> brightness(1) contrast(1.12) saturate(1.08)

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| JavaScript stránky | **spuštěn** v mock prohlížeči | projde, filtr se použije hned |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64 | 0 chyb |

---

## CO TESTOVAT

**1)** **Úvodní obrazovka** aplikace na výšku → co je na TV?
**2)** Spusť **PS1 na výšku** → co je na TV?
**3)** Zkus **Atari** nebo **Segu** → co je na TV?
**4)** Obraz hry — je kontrast lepší?

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | **celá úvodní obrazovka** s konzolemi | výřez |
| 2 | **jen obraz hry**, roztažený | celá aplikace nebo výřez |
| 3 | celé okno s Atari/Segou | výřez |
| 4 | ostřejší než v B110, kostky pořád slabší | vybledlé |

## CO POSLAT ZPĚT

Sedí kroky 1 až 3? A z logu jeden řádek:

    TV_KRESLENI prumer=... ms

Ten mi řekne, jestli vyhlazení není moc drahé.
