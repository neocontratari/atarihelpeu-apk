# B106 — DOLADĚNÍ OBRAZU, TENTOKRÁT OVĚŘENÉ (versionCode 154)

> **Tento kód je předpoklad. Čeká se na test.**

## Proč B105 shodil aplikaci

Dvě chyby v JavaScriptu té stránky, obě moje:

1. **Sahal jsem na prvky dřív, než byly získané.** Funkce se volala hned
   při načtení, ale posuvníky ostrosti a vyhlazení se získávaly až o tři
   řádky níž.
2. **Chybělo získání tlačítka `sF`** (celá obrazovka) — použil jsem ho,
   aniž bych ho kdy vzal ze stránky.

Když v té stránce spadne JavaScript, spadne WebView a s ním **celá aplikace**.

## Proč jsem to nechytil

Java se přeloží bez chyby, i když je v těch textových řetězcích JavaScript
nesmysl. Můj překlad to nemá jak poznat.

**Zařídil jsem to.** Od teď ten JavaScript vytáhnu z Javy, složím a **spustím**
v napodobenině prohlížeče. Výsledek u tohohle buildu:

    1) SPUSTENI: OK, NESPADLO
       filtr po startu: brightness(1) contrast(1) saturate(1)
    2) ostrost 50 -> brightness(1) contrast(1) saturate(1) url(#fx)
    3) reset -> jas=100 ostrost=0 vyhlazeni=0
    4) vyplneni -> cover | CELA OBRAZOVKA: oriznout

Tenhle test našel obě chyby. Je zapsaný v předávacím balíčku jako povinný.

## Co panel umí

    jas · kontrast · sytost · ostrost · vyhlazeni
    PUVODNI NASTAVENI
    CELA OBRAZOVKA: vyplnit / oriznout

Ostrost a vyhlazení počítá **grafika tvého PC**, ne telefon. Emulace se tím
nezpomalí a na mobilu se nic nemění.

Začni na **ostrost 30, vyhlazení 20**.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| JavaScript stránky | **spuštěn** v node s mock prohlížečem | projde, nespadne |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64 | 0 chyb |

---

## CO TESTOVAT

**1)** Zapni TV — **hlavně jestli aplikace nespadne**
**2)** Zkus **ostrost** a **vyhlazení**
**3)** **PUVODNI NASTAVENI** — vrátí všech pět?
**4)** Mobil beze změny?

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | aplikace běží, panel vpravo dole | spadne |
| 2 | obraz se mění hned | nic |
| 3 | všech pět na výchozí | jen část |
| 4 | mobil stejný | zhoršilo se |
