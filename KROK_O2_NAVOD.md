# KROK O2 — RAZÍTKO

Verze: **EMU10-O2-RAZITKO** (versionCode 32)

Parťáku, tenhle balíček **ještě nepřináší čistou cestu**. Je to to kolo,
které ji teprve umožní. Po O1 víme, že do telefonu přišla nová Java se
starým jádrem — takže dokud tohle nevyřešíme, opravujeme kód, který se
nespouští.

---

## Co uvidíš hned po instalaci

Otevři **SETTINGS**. Pod nadpisem OPTIONS budou dva řádky:

```
verze: EMU10-O2-RAZITKO (32)
O2 jadro prelozeno Jul 24 2026 02:15:33  |  gpuGles=BEZI krok=nezkouseno pixfmt=1
```

**Datum a čas na druhém řádku dosadí překladač v okamžiku, kdy jádro
skutečně přeloží.** Když se jádro znovu nepřeloží a v telefonu zůstane
staré, razítko zůstane staré. Uvidíš to na první pohled a nemusíš věřit
mně ani nikomu jinému.

Tři možnosti:

| Co uvidíš | Co to znamená |
|---|---|
| `O2 jadro prelozeno <dnešní datum>` | jádro je nové — jdeme dál |
| `STARE JADRO (razitko v knihovne chybi)` | jádro se zase nepřeložilo |
| `gpuGles=NEBEZI krok=eglCreateContext` | jádro je nové, ale grafika padá — a víme kde |

## Co se změnilo

### 1. Razítko jádra

Java a jádro se staví zvlášť. Do teď se nedalo poznat, že přišla nová
Java se starým jádrem. Teď to je vidět v panelu i jako druhý řádek logu.

### 2. Hlášení, jestli běží gpu-gles

Jádro má sedm míst, kde může nastavení grafiky selhat. Do teď se to psalo
jen do nativního logu — a ten se ve tvém logu z O1 neobjevil **ani jednou**.
Teď se zapamatuje, na kterém kroku to spadlo, a řekne to Javě.

### 3. Vypnutá paměť buildu v CI

**Tady vědomě porušuju pravidlo z předávacího protokolu „do `build.yml`
nešahej".** Ta paměť umí vrátit hotové výsledky z minula včetně
přeloženého jádra — což je nejpravděpodobnější důvod, proč ti přišlo
staré. Build bude o pár minut pomalejší.

Když si to nepřeješ, řekni a vrátím to. Ale pak nejspíš uvidíš staré
razítko a budeme to řešit jinudy.

### 4. Opravený můj parser z O1

V O1 stálo v každém řádku `zvuk-za-obrazem=?` a nikdo se nedozvěděl proč.
Teď se rovnou napíše, který údaj chybí.

---

## Co se NEZMĚNILO

Emulace, obraz, zvuk, cast, ovládání. Ani jeden řádek.

---

## Postup

1. Rozbal ZIP → složka `emu10_B`
2. Zkopíruj CELÝ obsah přes svůj repozitář → **Nahradit vše**
3. GitHub Desktop → popisek „krok O2 razitko" → **Commit** → **Push**
4. Počkej na zelenou → Actions → Artifacts → `app-debug` → do telefonu

---

## Co potřebuju

**Nemusíš nic hrát.** Stačí:

1. Nainstalovat
2. Otevřít **SETTINGS**
3. Vyfotit ty dva řádky nebo mi je opsat

To je celé. Když bude razítko dnešní a `gpuGles=BEZI`, pustíme se rovnou
do té čisté cesty.

Když si chceš navíc rozehrát Rebel Assault, log posílej — ale tentokrát
ho nepotřebuju k rozhodnutí. Rozhodne ten jeden řádek v panelu.

---

## Co bude dál

Podle toho, co razítko ukáže:

| Výsledek | Další krok |
|---|---|
| Jádro nové, gpu-gles běží | **O3 = čistá cesta.** Sdílená textura, pryč s kolečkem. |
| Jádro nové, gpu-gles nepadá | Opravím to konkrétní místo, které se v panelu jmenuje. |
| Jádro pořád staré | Příčina není v paměti buildu, ale v `pickFirsts` v `app/build.gradle`. |

Ta třetí možnost je má druhá hypotéza — v `app/build.gradle` je pravidlo
„když je knihovna na víc místech, vezmi první", které může tiše brát
starou kopii. Nechal jsem ho zatím být schválně: měnit dvě věci naráz
znamená nevědět, která zabrala.
