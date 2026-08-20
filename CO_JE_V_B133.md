# B133 — ZVUK V INTRU A ATARI S TV, PODRUHÉ (versionCode 181)

> **Tento kód je předpoklad. Čeká se na test.**
> B132 nepomohla ani u jednoho. Tohle je jiný přístup, ne přitvrzení toho starého.

## 1) Zvuk v intru — dělal jsem to jinak než to, co v aplikaci funguje

Z tvého logu bylo vidět, že **zvuk nebyl uspaný**:

```
BUILD2SA24 INTRO_ZVUK vytvoren, stav=running
BUILD2SA24 INTRO_ZVUK BEZI
```

Takže moje oprava v B132 řešila něco, co nebylo rozbité. Šel jsem hledat jinam
a našel to v tvé vlastní aplikaci:

**Emulátor Atari přes ten samý WebView zvuk vydává.** A dělá to úplně jinak
než já:

```js
sp = ac.createScriptProcessor(4096,0,1);   // JEDEN trvale připojený uzel,
sp.onaudioprocess = ...                     // který si vzorky počítá sám
```

Já jsem pro každý tón vyráběl `OscillatorNode` a zase ho rušil. Kontext hlásil
`running`, ale ven nešlo nic.

**Přepsal jsem zvuk na `ScriptProcessor`, stejně jako to má Atari** — protože
o tom způsobu z tvé vlastní aplikace víme, že v tomhle WebView hraje. Zvuky
si teď počítám sám: obdélník, trojúhelník, pila, šum, každý s doznívajícím
obalem.

**Ověřil jsem, že z toho opravdu lezou vzorky**, ne ticho:

```
vzorků spočítáno:    1 708 032
z toho nenulových:   1 207 540 = 70,7 %
největší výchylka:   1,000
```

To je otázka, kterou můj předchozí test vůbec nepokládal — díval se jen na to,
kolik oscilátorů se vyrobilo, ne jestli z nich něco leze.

## 2) Atari s TV — zpomalení nestačilo, snímek je moc drahý

B132 tempo **snížila z 29 na 13,4 snímku za vteřinu** (ověřeno z tvého logu),
takže ta změna zabrala — ale kousalo se dál. Důvod: **jeden snímek je sám
o sobě moc drahý.**

```
720 × 1336 = 961 920 bodů se čte zpátky z okna
a celá bitmapa 3,7 MB se kopíruje na HLAVNÍM vlákně
```

Na tom samém vlákně, kde běží emulátor.

**Atari se teď snímá v polovičním rozlišení:**

```
720 × 1336  ->  360 × 668     25 % bodů
3,7 MB      ->  0,9 MB        čtvrtinová kopie
```

Na TV bude obraz měkčí, ale emulátor má konečně čas počítat. Týká se to
**jen Atari** — PS1 a Sega dávají snímek z jádra a snímání okna se jich netýká.

## 3) A dopsal jsem měření, které se dělalo a nikam nepsalo

V kódu se už dávno počítalo, kolik stojí ta kopie bitmapy na hlavním vlákně —
ale **nikdy se to nikam nezapsalo**. Takže při „kouše se to" nebylo z čeho
vyjít. Teď je to v logu:

```
BUILD2SA25 TV_CENA_HLAVNIHO_VLAKNA kopieBitmapy=..ms pixely=..ms yuv=..ms ...
```

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci — **hraje v intru zvuk?**
**3)** Zapni **WEB TV**, jdi do **ATARI 130XE** a chvíli hraj.
**4)** Vypni TV, Atari znovu — má být jako dřív.
**5)** PS1 a SEGA s TV — beze změny.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | hraje hudba i klapání kláves | ticho |
| 3 | **Atari běží plynule**, TV měkčí a méně plynulá | pořád se kouše |
| 4 | jako dřív | horší |
| 5 | beze změny | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej v něm:

```
BUILD2SA24 INTRO_ZVUK po 5 s: stav=... tonu=... znejicich=...
BUILD2SA25 TV_CENA_HLAVNIHO_VLAKNA ...
```

**Kdyby zvuk pořád nešel**, ten první řádek řekne, jestli se tóny vůbec
vyrábějí. Když v něm bude `tonu=0`, chyba je u mě v tom, kdy zvuk volám.
Když tam bude velké číslo a přesto ticho, je to WebView a půjdu jinou cestou.


---

# Předchozí kola (B131)

## Osmé kolo — tlačítko INTRO v nabídce OPTIONS

Vpravo nahoře v rozcestníku, v kolečku **OPTIONS**, přibylo pod
`SPUSTIT WEB TV` tlačítko:

```
PUSTIT INTRO
```

Jde jím intro spustit kdykoli, a protože se přehrává ve stejném okně
jako všechno ostatní, **jde i na WEB TV**.

### Na TV to půjde samo — a je to doložené

Prošel jsem cestu obrazu:

```java
PixelCopy.request(getWindow(), ...)
```

Snímá se **celé okno aplikace**, ne konkrétní stránka. Ta jediná podmínka
na adresu (`emu_ps1`, `emu_sega`) tam je proto, že tyhle dva dávají snímek
přímo z jádra a snímání okna obcházejí. Když se na nich není, TV se vrátí
ke snímání okna — a intro je přesně takový případ.

### Jedna past, kterou jsem ošetřil

Intro si při doběhnutí hlásí do Javy „ukázáno", aby při návratu z her
neotravovalo. Kdyby to udělalo i při ručním spuštění, hlásilo by to
zbytečně podruhé.

Intro proto pozná, že bylo puštěno ručně (`?znovu=1`), a v tom případě
to nehlásí — do logu se zapíše `INTRO_NA_POZADANI`.

Ověřeno spuštěním obou případů:

```
při STARTU aplikace       doběhl v 62,0 s, hlásí "ukázáno": ANO
ručně z nabídky OPTIONS   doběhl v 62,0 s, hlásí "ukázáno": ne
```

### Pozor při testu s TV

Snímání okna běží na hlavním vlákně — u Atari jsem naměřil asi 29 ms na
`PixelCopy` plus 16 ms na kódování H.264. Intro k tomu přidá svoje
kreslení. **Bez TV to nevadí, s TV to může být znát.**

Kdyby ano, v `intro/index.html` je:

```js
var PLAZMA_PLYNULE = true;   // na false = 11x rychlejší
```

## Sedmé kolo — READY před psaním

Na Atari po startu BASICu nejdřív svítí **READY** a pod ním kurzor —
teprve pak píšeš. To jsem tam neměl a je to ta ikonická věc.

Teď to jde takhle:

```
READY
█            1,4 s jen bliká kurzor, žádné klapání
10 GRAPHICS 0          pak se začne psát
20 SETCOLOR 2,12,2:SETCOLOR 4,12,2
...
```

`READY` se objeví **najednou a beze zvuku** — nepíše ho uživatel, vypisuje
ho BASIC. Klapání kláves začne až u prvního řádku programu.

Scéna je o tu pauzu delší, celý film má teď **62 vteřin**.

## Šesté kolo — původní písmo z Atari

**Nemusel jsem ho trefovat. Mám ho.** Znaková sada je v ROM, kterou jsi
dodal: `$E000-$E3FF`, 1024 bajtů, 128 znaků po osmi bajtech.

```
index $21  ->  A          index $10  ->  0
   ...##...                  ..####..
   ..####..                  .##..##.
   .##..##.                  .##.###.
   .##..##.                  .###.##.
   .######.                  .##..##.
   .##..##.                  ..####..
```

Text v obou monitorech se teď kreslí **bod po bodu z té sady**, ne písmem
z prohlížeče. Kurzor je plná buňka, jak to Atari dělá. Převod ASCII na
vnitřní kód obrazovky je ten, co má Atari: `$20-$5F` mínus `$20`,
malá písmena beze změny.

Ověřeno v zabaleném souboru: **1024 bajtů, shoduje se s ROM bajt po bajtu**.

### Jak to nestojí výkon

Celá sada se jednou nakreslí do zásobního plátna (4 body na jeden bod
Atari) a pak už se jen kopírují výřezy. Kreslit 64 bodů na každý znak
každý snímek by bylo drahé.

Nejtěžší scéna spadla z 51 na **37 ms na snímek** — kreslení znaků
z hotového obrázku je levnější než sazba písma.

### Dvě chyby, které jsem si u toho našel

**1. Atlas jsem si držel jen jeden.** Text se kreslí dvakrát — jednou stín,
jednou barva — takže se při každém znaku přepínalo mezi dvěma barvami
a celá sada se překreslovala znovu. Render se kvůli tomu ani nedoběhl.
Teď se drží atlas pro každou barvu zvlášť.

**2. Zaokrouhlení bodu na celé číslo srazilo monitor na osminu.** Chtěl
jsem, aby jeden bod Atari vyšel na celé obrazovkové body. Jenže na výšku
vycházela buňka 15,3 bodu, tedy 1,9 bodu na jeden bod Atari — a
zaokrouhlení dolů z toho udělalo 1, takže buňka spadla na 8 bodů
a monitor se scvrkl. Teď se atlas staví v pevném rozlišení a při kreslení
se zvětší na skutečnou buňku.

## Páté kolo — podle tvých připomínek

**Obrazovka Atari zmenšená.** Na šířku ze 49 % obrazu na 38 %, na výšku
na 25 %. Kolem ní je vidět, co lítá — hvězdy jsem zesílil a zrychlil,
bobíků je víc a logo jsem taky zmenšil, aby pozadí nezakrývalo.

**Řádky mají rozestup.** Text se teď kreslí znak po znaku, každý do své
buňky, takže mřížka je přesná a velikost písma se dá volit nezávisle na
šířce buňky.

**Kurzor sedí těsně za textem.**

**SBÍRKA v Seze** vede rovnou na `https://atarihelp.eu/?page_id=1003`.
Dlouhý stisk na tom tlačítku pořád otevírá nabídku výkonu — na to jsem nesáhl.

### Dvě věci, které jsem u toho našel u sebe

**1. Můj testovací nástroj lhal.** Text jsem vodorovně natahoval přes
`ctx.scale(1.27, 1)`, aby znaková buňka měla poměr 0,8 jako na Atari.
Ověřil jsem to samostatným pokusem:

```
text bez natažení      rozsah 11 – 128 bodů
text se scale(2,1)     rozsah 12 – 139 bodů   ← má být dvojnásobek
```

**V mém testovacím plátně se `scale` na text neprojeví.** V prohlížeči ano.
Znamenalo by to, že ti posílám obrázky, které neodpovídají tomu, co uvidíš
na telefonu — a já bych to nikdy nezjistil.

Zahodil jsem natahování úplně. Šířka buňky se odvodí ze **skutečně
změřeného** písma a výška z poměru 0,8, takže obrazovka vyjde 4:3 sama.
Co vidím já, uvidíš i ty.

**2. Špatný poměr znakové buňky.** Měl jsem šířku znaku 0,601 a výšku
řádku 1,34 — poměr 0,45, a monitor tím vycházel **na výšku**. Na šířkovém
telefonu z toho byl vysoký úzký obdélník s písmem jako pod lupou.

Atari GRAPHICS 0 má 40×24 buněk po 8×8 bodech, tedy 320×192, a na televizi
4:3 je bod 0,8× tak široký jako vysoký — takže buňka má poměr 0,8 a celá
obrazovka vyjde 32:24 = 4:3.

### Výkon

Text se kreslí do vlastního plátna a **jen když se změní**. Při 52 ms na
znak je to nejvýš 20× za vteřinu místo 60×.

## Co jsem opravil podle tebe

**Dvě loga jsou pryč.** Neopravoval jsem odraz — **smazal jsem ho**.
A s ním i celou starou scénu, ve které byl, ať se nemůže vrátit.
Ověřeno: `odrazLoga(` se v souboru vyskytuje **nulakrát**.

**GOLD BETA - JEN PRO KKT** smazáno. Nulakrát.

**Plovoucí text smazán.** Nulakrát.

**Slunce smazáno.** Nulakrát.

Vyhodil jsem rovnou i všechny funkce, které se už nepouštěly —
`hlavni`, `logoPrilet`, `beh`, `scroller`, `odrazLoga`, `slunce`.
Mrtvý kód je past: za měsíc to někdo zapojí zpátky a diví se.

## Vsazená obrazovka s duhou

Ve třetím okně je dole **rámeček jako monitor** a v něm běží program:
duha přes celou plochu a přes ni vypsaný text. Duha se posouvá.

Na Atari se to dělá tak, že se mění barva pozadí na každém řádku
(`POKE 710` v přerušení display listu). Tady je to nakreslené —
je to fake, jak jsi sám řekl — ale vypadá to stejně a chová se to
stejně: text se dopisuje s kurzorem, jako by program běžel.

## Klávesy klapou jako Atari

Předtím jsem tam měl **pípání** a to bylo špatně. Atari pro klávesu
žádný tón nedělá — OS jen **klapne reproduktorem** (bit 3 registru
CONSOL). Je to krátký cvak, ne pípnutí.

Předělal jsem to na velmi krátký šum s ostrým náběhem. RETURN klape
jinak než písmeno.

## Ověřeno u mě

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 42 tříd |
| C++ Atari, PS1, Sega (aarch64) | 4/4 |
| linkování PS1+Sega | 31/31 |
| JS rozcestníku, intra i HELP | 0 chyb |
| film spuštěný proti skutečnému plátnu | doběhne za 60,5 s, přesměruje |
| původní Atari | bajt po bajtu netknuté |

**Výkon:** nejtěžší scéna 53 ms na snímek v **softwarovém** vykreslování
mého testu. Telefon má GPU, kde je roztažení plazmy prakticky zadarmo.
Kdyby se přesto sekalo, je v `intro/index.html`:

```js
var PLAZMA_PLYNULE = true;   // na false = kostičkovaná plazma, 11x rychlejší
```

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci a **nech film doběhnout celý**.
**3)** Spusť znovu (ukončit a otevřít) — má naskočit zas.
**4)** Během filmu zmáčkni **PŘESKOČIT**.
**5)** Vrať se z hry do nabídky — film **nemá** naskočit.
**6)** **ATARI**, **PS1**, **SEGA** — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | tři okna za sebou, **jedno jediné logo**, dole běžící program s duhou | dvě loga, chybí okno, seká se, ticho |
| 3 | naskočí znovu | nenaskočí |
| 4 | okamžitě do nabídky | nic |
| 5 | nenaskočí | naskočí |
| 6 | jako dosud | jakákoli změna |

## CO MI ŘEKNI

1. **Je tam pořád někde druhé logo?** Mělo by být jedno.
2. **Klapou klávesy jako Atari?** Dělal jsem to po paměti.
3. **Neseká se to?**
4. **Sedí ta duha?** Je to fake, ale má vypadat jako běžící program.

## CO POSLAT ZPĚT

Log přes **ULOZIT LOG A ODESLAT** na obrazovce HELP.

---

## Atari v C++ — kde se skončilo

Práce **pozastavená** na tvůj pokyn, celý stav je v
`PREDAVACI_BALICEK_PS1.txt`.

**Běží:** procesor (1 024 000 instrukcí, 0 rozdílů), paměť (16 252 928
čtení, 0 rozdílů), WSYNC, zavaděč XEX, display list, DLI, OS nabootuje,
BASIC naskočí, **self-test z ROM se vykreslí celý**, Decathlon se nahraje.

**Neběží:** vykreslení Decathlonu, kolize, zvuk, VBXE.

**Poučení v protokolu:** vrstvy 1 a 2 jsem překládal z JavaScriptu a
vyšly napoprvé. Vrstvu 3 jsem začal psát znovu a za den z toho bylo
**osm vlastních chyb**, které v původním JS nejsou. Další krok je
vrstvu 3 **přeložit**, ne psát.

---

## CO TESTOVAT V B132

**1)** Build v Actions.
**2)** Spusť aplikaci — **hraje v intru zvuk?**
**3)** Zapni **WEB TV**, pak jdi do **ATARI 130XE** a chvíli si zahraj.
       Kouše se obraz nebo zvuk?
**4)** Vypni TV a zkus Atari znovu — má být jako dřív.
**5)** V nabídce OPTIONS zmáčkni **PUSTIT INTRO** — hraje zvuk i teď?
**6)** PS1 a SEGA s puštěnou TV — mají být beze změny.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | v intru hraje hudba i klapání kláves | ticho |
| 3 | **Atari běží plynule**, TV je méně plynulá (asi 15 snímků/s) | Atari se pořád kouše |
| 4 | jako dřív | horší než dřív |
| 5 | hraje | ticho |
| 6 | beze změny | jakákoli změna |

**Krok 3 je ten hlavní.** Je v něm vědomý kompromis: TV bude méně plynulá,
aby emulátor běžel. Řekni, jestli je ten poměr správně.

## CO POSLAT ZPĚT

Log přes **ULOZIT LOG A ODESLAT**. Hledej v něm řádky:

```
BUILD2SA24 INTRO_ZVUK ...
```

Ty mi řeknou, jestli se zvuk vyrobil a jestli běží — i kdyby nehrál.
