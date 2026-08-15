# B108 — VYLADĚNÍ PŘED KOMPRESÍ (versionCode 156)

> **Tento kód je předpoklad. Čeká se na test.**
> Stojí na B107 (tvůj funkční základ). Mobil ani jádro netknuté.

## Měl jsi pravdu a já to zkusil špatně

Ptal ses, proč jde stará videokazeta vyladit na HD kvalitu, a náš čistý
obraz ne. Odpověď: **jde to, jen záleží KDY se to udělá.**

- **Video z kazety** se čistí **PŘED** kompresí, na originálu — filtr má
  k dispozici všechnu informaci, co na pásce je.
- **Já to v B105 zkusil AŽ POTOM**, na obraze, kterému už detaily chyběly.
  Proto to zvýraznilo jen chyby komprese.

A u tebe je to ještě lepší než u kazety: **obraz z jádra je čistý originál,
žádný šum, žádná ztráta.**

## Co je v B108

Vyladění se dělá **při kreslení na plátno, ještě před enkodérem**:

- **mírné vyhlazení** při zvětšení
- **kontrast 1,10** se srovnáním černé
- **sytost 1,12**

Počítá to grafika telefonu, takže to skoro nic nestojí — v logu měl telefon
rezervu 10 ms ze 16.

**A tady je ten protiintuitivní kus:** mírně měkčí obraz se komprimuje
**čistěji**, protože H.264 má s ostrými schody problém. Takže výsledek na
televizi bude paradoxně vypadat ostřeji, ne rozmazaněji — přesně jako ten
starý film převedený na HD.

## Můžeš to porovnat sám

V panelu je nové tlačítko **VYLADENI OBRAZU: zapnuto / vypnuto**. Přepni ho
za běhu a uvidíš rozdíl okamžitě, na stejné scéně.

Pokud se ti bude líbit vypnuté, řekni a nastavím to jako výchozí — nebo to
celé vyhodím.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| JavaScript stránky | **spuštěn** v mock prohlížeči, i s novým tlačítkem | projde |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64 | 0 chyb |

---

## CO TESTOVAT

**1)** Zapni TV, pusť **NFS** a nech běžet intro
**2)** Zmáčkni **VYLADENI OBRAZU** a sleduj rozdíl na stejné scéně
**3)** Přepni ještě párkrát tam a zpět
**4)** Mobil — nesmí se nic změnit

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz čistší než v B107, kostky slabší | horší |
| 2 | rozdíl je hned vidět, tlačítko změní nápis | nic se nestane |
| 3 | přepíná spolehlivě | zasekne se |
| 4 | mobil beze změny | zhoršilo se |

## CO POSLAT ZPĚT

Který stav vypadá líp — **zapnuto, nebo vypnuto**? A vypadá to jako film,
nebo pořád jako zvětšené pixely?
