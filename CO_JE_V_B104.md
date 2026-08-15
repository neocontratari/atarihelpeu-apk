# B104 — CELÁ OBRAZOVKA (versionCode 152)

> **Tento kód je předpoklad. Čeká se na test.**

## Ty pruhy byla moje chyba z B103

V B103 jsem obraz pro TV změnil na **1280×960**, aby se zvětšoval přesně
dvakrát. To sedělo na zdroj, ale nesedělo na tvoji obrazovku: **1280×960 je
poměr 4:3, zatímco televize i monitor jsou 16:9.** Odtud černé pruhy po
stranách.

Zpátky na **1280×720**, tedy 16:9 — obraz vyplní obrazovku.

Ostrost tím neztratíme: tu drží to, že se při zvětšení **nefiltruje**
(z B100), takže pixely zůstanou ostré i při jiném poměru. To bylo to
podstatné, ne přesný násobek.

## To tlačítko tam už bylo

Reset je pod posuvníky — na tvých screenech je vidět jako **„puvodni"**.
Pojmenoval jsem ho srozumitelněji: **PUVODNI NASTAVENI**.

## A přidal jsem přepínač výplně

Pod ním je teď **CELA OBRAZOVKA: vyplnit / oříznout**.

- **vyplnit** — celý obraz se vejde (může nechat pruhy u netypických poměrů)
- **oříznout** — vyplní obrazovku vždycky, ale ukrojí kousek okrajů

Nastavení se pamatuje, stejně jako jas a kontrast.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Zapni TV — jestli obraz **vyplní obrazovku bez pruhů**
**2)** Zkus tlačítko **PUVODNI NASTAVENI** po rozladění posuvníků
**3)** Zkus přepínač **CELA OBRAZOVKA**
**4)** Zkontroluj ostrost — nemělo by se to zhoršit proti B103

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz přes celou šířku, bez černých pruhů | pruhy |
| 2 | jas, kontrast i sytost skočí na výchozí | nic se nestane |
| 3 | obraz přepne mezi „vejde se" a „vyplní" | nic |
| 4 | ostrost jako v B103 | rozmazané |

## CO POSLAT ZPĚT

Je obraz přes celou obrazovku a funguje reset?
