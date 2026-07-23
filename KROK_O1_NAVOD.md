# KROK O1 — MĚŘENÍ (nic se neopravuje)

Parťáku, tenhle balíček **nic nezlepšuje**. Jenom začne psát do logu čísla,
která tam do dneška nebyla — a bez kterých bych další krok dělal poslepu.

Verze: **EMU10-O1-MERENI** (versionCode 31)

---

## Co se změnilo (čtyři soubory, nic víc)

### 1. Do logu přibylo měření odstupu zvuku

Každých 5 vteřin, dokud běží PS1, se zapíše řádek:

```
O1 ODSTUP zvuk-za-obrazem=123 ms | vyrobeno=... zahozeno=... prehrano=...
   fronta=... orezu=... vypadku=... snimku=...
```

**Odstup** = o kolik je zvuk, který právě slyšíš, pozadu za obrazem na displeji.
Počítá se poctivě, žádný odhad:

```
odstup = (co jádro vyrobilo − co se zahodilo − co se přehrálo) / 44100
```

**Orezů** je to číslo, kvůli kterému to celé dělám. V jádře je pojistka:
když zvuková fronta naroste přes ~400 ms, **naráz vyhodí až 267 ms zvuku**.
To se počítá od začátku projektu, ale **do logu se to nikdy nezapisovalo** —
šlo to jen do JavaScriptu. Takže za všech tvých 2600 testů ti tohle číslo
neproběhlo před očima ani jednou.

Když se orez stane mezi dvěma zápisy, na konci řádku vyskočí:

```
<<< OREZ ZVUKU x1 (zvuk skocil dopredu)
```

### 2. Verze je konečně vidět v aplikaci

Pravidlo 7 z předávacího protokolu říká, že popisná verze má být vidět
v aplikaci. **Nebyla — nikde.** Ani v Javě, ani v HTML. Jediné místo, kde
ta verze žila, byl `build.gradle` na tvém počítači.

Teď je:
- v **SETTINGS** hned pod nadpisem OPTIONS
- a jako úplně první řádek každého logu

### 3. Opravena zafixovaná IP adresa

`index.html` psal natvrdo:

```
toast('WEB TV zapnuto — 192.168.0.34:8765')
```

Přitom Java tu adresu umí zjistit sama (`napTvWebLocalIp` projde síťová
rozhraní) a JavaScript si skutečnou adresu **vyzvedne a zaloguje** — jen ji
nezobrazí. Aplikace znala pravdu a na obrazovku napsala něco jiného.

A lhala ve dvou číslech, ne v jednom: když je port 8765 obsazený, server
spadne na náhodný volný port — text říkal 8765 dál.

Teď se ukáže skutečná adresa, kterou appka opravdu poslouchá.

### 4. Verze +1

`versionCode 30 → 31`, `versionName` popisný podle pravidla 7.

---

## Co se NEZMĚNILO

Emulace, obraz, zvuk, cast, ovládání, výkonnostní třídy — **nic**.
Nový je jeden čítač v jádře (jen se přičítá, nic neřídí) a jedna smyčka,
která každých 5 vteřin zapíše řádek do logu.

Když bude appka po tomhle buildu chodit jinak, je to moje chyba a řekni
mi to rovnou.

---

## Postup

1. Rozbal ZIP → složka `emu10_B`
2. Zkopíruj CELÝ obsah přes svůj repozitář → **Nahradit vše**
3. GitHub Desktop → popisek „krok O1 mereni" → **Commit** → **Push**
4. Počkej na zelenou → Actions → Artifacts → `app-debug` → do telefonu

---

## Co budu potřebovat

**Rozehraj Rebel Assault II na ~2 minuty. Ať v tom je i filmová scéna
s mluvením** — tam ti to nesedí nejvíc a tam to nejlíp uvidím.

Pak mi pošli log z `8765/log`.

### A jedna otázka, na kterou z kódu odpověď nevyčtu

**To ujíždění zvuku — je plynulé, nebo to občas skočí?**

- **Plynulé** → odstup v logu poroste rovnoměrně, orezů bude málo
- **Skoky** → uvidíme `<<< OREZ ZVUKU` přesně v tom místě

Tvoje ucho to rozliší dřív a líp než jakékoli číslo. Až to poslechneš,
napiš mi, cos slyšel — porovnám to s tím, co ukázal log.

---

## Co uvidím v logu a co z toho vyvodím

| Co uvidím | Co to znamená | Kam jdu dál |
|---|---|---|
| Odstup roste, orezů 0 | Odběr zvuku nestíhá | Oprava v odběru |
| Orezy střílí pravidelně | Fronta přetéká, zvuk skáče | Oprava té pojistky |
| Odstup stabilní, orezů 0 | Příčina není ve zvuku | Rovnou na grafiku |

Podle toho se rozhodne krok O2 — ne podle toho, co si myslím teď.
