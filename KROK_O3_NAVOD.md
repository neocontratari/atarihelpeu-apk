# KROK O3 — NOVÉ JMÉNO KNIHOVNY

Verze: **EMU10-O3-NOVE-JMENO** (versionCode 33)

## Co ukázalo O2

Razítko zafungovalo přesně jak mělo — a řeklo mi, že **moje hypotéza byla
špatná**. Vypnutí paměti buildu nepomohlo. Jádro je pořád staré.

Takže staré jádro nepřichází z paměti buildu. Přichází odjinud.

## Co jsem udělal místo dalšího hádání

Přejmenoval jsem knihovnu z `napps1core` na **`napps1core3`**.

Proč to rozhodne nadobro: aplikace teď žádá jádro pod novým jménem.
Jakákoli stará kopie se jmenuje po staru — **a pod novým jménem se
nemůže vydávat.** Takže existují jen dva možné výsledky a oba jsou
jednoznačné:

| Co uvidíš v SETTINGS | Co to znamená |
|---|---|
| `O3 jadro prelozeno <datum a čas>` | jádro je **nutně čerstvé** — jinak by se nenačetlo |
| `JADRO SE VUBEC NENACETLO -> CMake nic neprelozil` | jádro se vůbec nestaví, a víme to jistě |

Žádná třetí možnost. Žádné "asi".

## Prosba — 30 vteřin a ušetří nám to kolo

Ve složce s repozitářem v PC dej vyhledat: **`*.so`**

Když se něco najde (třeba `libnapps1core.so` ve složce `jniLibs`), napiš
mi cestu. Bude to nejspíš přesně ten viník — stará ručně přiložená
knihovna, kterou žádné "Nahradit vše" nesmaže, protože v mém balíčku není.

Když se nenajde nic, taky mi to napiš. Je to stejně cenná informace.

## Postup

1. Rozbal ZIP → složka `emu10_B`
2. Zkopíruj CELÝ obsah přes repozitář → **Nahradit vše**
3. GitHub Desktop → „krok O3 nove jmeno" → **Commit** → **Push**
4. Zelená → Actions → Artifacts → `app-debug` → do telefonu
5. **SETTINGS** → vyfoť ty dva řádky

Hrát nemusíš. Rozhodne ten jeden řádek.

## Poctivě k riziku

Přejmenování je zásah do stavby, ne do emulace. Když se něco pokazí,
pokazí se to hlasitě — appka řekne, že se jádro nenačetlo. Nemůže z toho
vzniknout tichá vada, která by ti zkazila obraz nebo zvuk.

Build v CI necháváme bez paměti (z O2). Až bude jádro prokazatelně
čerstvé, vrátím to zpátky, ať nestavíš zbytečně dlouho.

## Co bude dál

Až razítko ukáže dnešní datum, uvidíme poprvé v životě projektu,
jestli gpu-gles na tvém telefonu naskočí. A pak jde O4 = čistá cesta:
jádro → grafika → mobil i TV, bez sestupu do procesoru.
