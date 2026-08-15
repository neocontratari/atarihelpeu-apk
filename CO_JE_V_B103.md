# B103 — PŘESNÝ DVOJNÁSOBEK + RUČNÍ DOLADĚNÍ (versionCode 151)

> **Tento kód je předpoklad. Čeká se na test.**

## Ty dvě fotky skutečné PS1 byly cenné

Podívej se na tu druhou — na menu s auty. Vidíš ty **čtvercové bloky**
i na skutečné konzoli? To jsou artefakty MPEG videa přímo z herního disku.
Takhle to hra opravdu má, s emulací to nesouvisí. Takže část toho, co jsme
honili, tam prostě patří.

Ale zbytek se zlepšit dal.

## Poslední skutečná ztráta: zvětšení 1,5×

    zdroj z her:   640 x 480
    cil pro TV:   1280 x 720

    vodorovne  640 -> 1280 = presne 2x   (kazdy bod na ctverec 2x2, ostre)
    svisle     480 ->  720 = 1,5x        <- TADY

Při 1,5× připadnou na každé **dva body tři** — jeden se zdvojí a sousední ne.
Vznikne nepravidelný vzor, který H.264 špatně komprimuje, a v tmavých
plochách z něj dělá kostky.

Teď je to **1280×960, tedy přesně 2× v obou směrech.** Každý bod PlayStation
se zvětší na čistý čtverec. Prohlížeč si poměr dorovná sám, takže se na
obrazovce nic nezmění.

Datový tok zůstává 27 Mbit/s — při pravidelném zdvojení se obraz komprimuje
líp, takže na stejnou kvalitu stačí míň bitů na bod.

## A to doladění, cos chtěl

V prohlížeči je teď vpravo dole **panel s jasem, kontrastem a sytostí**.
Objeví se při pohybu myší a po pár vteřinách zmizí. Nastavení se pamatuje.

Udělal jsem to takhle schválně: různé hry mají různě tmavý obraz, takže
nastavit to natvrdo v kódu by u jiných her uškodilo. Takhle si to nastavíš
podle sebe a podle své televize.

Na mobilu jsem nechal obraz **beze změny**, jak jsi chtěl.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** NFS na TV — tmavé scény
**2)** Pohni myší nad obrazem a zkus **panel vpravo dole**
**3)** Zkontroluj, že se mobil nezhoršil

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | kostky **znovu míň**, obraz čistší | stejné / kouše se |
| 2 | panel jde ovládat, změna je vidět hned | panel není |
| 3 | mobil beze změny | zhoršilo se |

Kdyby se to začalo kousat, je to tím větším obrazem (o třetinu víc bodů) —
řekni a stáhnu tok na 20 Mbit/s.

## CO POSLAT ZPĚT

Jsou kostky menší? A funguje ten panel?
