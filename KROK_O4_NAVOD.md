# KROK O4 — CELÝ OBRAZ

Verze: **EMU10-O4-CELY-OBRAZ** (versionCode 34)

Poprvé to není měření. Tohle je oprava.

---

## 1. Chybějící kusy obrazu

Ve funkci, která dostává obrazová data do plátna, byly tři podmínky,
které zápis **potichu zahodily**:

```c
if (yy1 <= band0 || yy0 >= band1) return;   // mimo řádky displeje
if (h < 4) return;                          // cokoli tenčího než 4 řádky
if (!ok) return;                            // mimo sloupce displeje
```

Měly šetřit práci tím, že hádaly, co je obraz a co texturová data.
Hádaly špatně. A co zahodily, se do plátna už nikdy nedostalo — na tom
místě zůstal ležet starý obsah. To je „grafika není celá".

**Proč to log nemohl ukázat:** zapisovalo se až ZA těmi bránami. Log
tedy obsahoval výhradně to, co prošlo. Zahozené v něm z principu být
nemohlo. Proto pořád vycházelo, že je všechno v pořádku.

**Teď projde všechno.** Plátno je přesná kopie paměti. Obraz je celý
z principu, ne z odhadu.

Místo zahazování se jen počítá, co by staré brány byly vyhodily:

```
O4 BRANY celkem=3500 drive_zahozeno: radky=812 tenke=1044 sloupce=190
```

Ta čísla ti řeknou, o kolik obrazu jsi přicházel.

## 2. Blikání

Čtecí kotva občas přijde jako `srcX=480` při šířce 640. To se do VRAM
(1024) nevejde, a kód to potichu utnul na 384 — takže se přečetla
oblast 384 až 1023, kde leží texturové stránky. Odtud ty znakové sady.

V logu to mělo přesný tvar blikání: 2× `srcX=480`, pak 4-13× `srcX=0`,
dokola.

Taková kotva není obraz, ale přechodový stav při přehození bufferu.
Nový snímek se prostě nepošle a na obrazovce zůstane předchozí. Při
dvou snímcích z patnácti to oko nepozná.

```
O4 KOTVA_MIMO srcX=480 srcY=0 rb=640x480 - snimek zahozen (celkem 34)
```

---

## Riziko, které přiznávám dopředu

Nahrávat všechno je víc práce než nahrávat výběr. **Může to zpomalit.**
Kolik přesně, ti dopředu neřeknu — proto ta počítadla.

Pořadí mi ale přijde jasné: **nejdřív celý obraz, potom rychlý.**
Ladit rychlost rozsypaného obrazu nemá smysl.

Když to bude pomalé, uvidíme v číslech přesně kde, a zrychlíme to
poctivě — dávkováním, ne zahazováním obrazu.

---

## Postup

1. Rozbal ZIP → složka `emu10_B`
2. Zkopíruj CELÝ obsah přes repozitář → **Nahradit vše**
3. GitHub Desktop → „krok O4 cely obraz" → **Commit** → **Push**
4. Zelená → Actions → Artifacts → `app-debug` → do telefonu

## Co mě zajímá

Rozehraj to a řekni mi očima, ne logem:

1. Je obraz **celý**? Nechybí kusy, nejsou tam znakové sady?
2. **Bliká** to ještě?
3. Je to **pomalejší** než dneska ráno?

Log pošli taky, ale rozhodne to, co uvidíš ty.
