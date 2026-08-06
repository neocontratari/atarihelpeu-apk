# B83 — ČISTÉ JÁDRO, PROVĚŘENÝ VYKRESLOVAČ (versionCode 131)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**
> Ověřeno: Java `javac` proti `android.jar`, C++ `clang` pro aarch64 proti
> **skutečnému** `jni.h`, a celé jádro **přeloženo pro ARM64 křížovým
> překladačem — 0 chyb, výsledek je opravdu ARM aarch64.**

## Nejdřív rovnou: co je a co není FAKE

Prošel jsem to a spočítal:

    libpcsxcore         41 898 radku   procesor, GTE, zvuk, MDEC, CD-ROM
    plugins/gpu_neon    19 377 radku   HOTOVY vykreslovac ... NEPREKLADAL SE
    plugins/gpu_naples2  1 291 radku   rucne psany ... a TENHLE se prekladal

**Jádro fake není.** Je to PCSX-ReARMed a `mdec.c` (dekodér videa) se
překládá — takže ta část `navod.txt`, která tvrdí, že MDEC je „softwarový
interpolátor", není pravda. Stejně tak „řídit vykreslování zápisy do
registru GPU STAT" není, jak PlayStation funguje.

**Ale ve vykreslovači jsi měl pravdu.** Devatenáct tisíc řádků prověřeného
kódu leželo nepoužitých a nahradilo je dvanáct set řádků psaných ručně.
Ten ruční neuměl všechno, co PlayStation dělá — odtud kostičky ve Formuli,
artefakty ve Star Wars a blikající dema.

## Co je v B83

**Překládá se `gpu_neon`.** Je to součást jádra PCSX-ReARMed a je to
**tentýž vykreslovač, který kreslil ten referenční obraz BIOSu**, co jsem
ti posílal jako správný.

Aplikace volala osm funkcí, které patřily jen k tomu ručnímu. Ty dodává
můstek `nap_most.c` (70 řádků), aby se nemuselo přepisovat půl aplikace.

Cesta obrazu se nemění a **pořád končí na GPU**:

    jadro -> gpu_neon kresli do videopameti
          -> gpulib preda snimek
          -> nativni plocha ho nakresli pres OpenGL ES

Ručně psaný vykreslovač zůstává v repozitáři, jen se nepřekládá — nic se
nezahazuje.

## Duch BIOSu v celé aplikaci — opraveno

Plocha leží nad stránkou (jinak by ji na výšku zakryla neprůhledná grafika
konzole), takže mimo obrazovku PS1 prosvítala všude.

Sledování, které ji má schovat, viselo **uvnitř smyčky pro TV** — takže
**bez zapnuté TV se vůbec nespouštělo.** Teď běží samostatně, každých
300 ms, nezávisle na TV.

## Co tenhle build neřeší

- **Načtení další hry** bez restartu. Vím o tom, je to zapsané.
- **ISO CD** předává hru jako popisovač souboru místo skutečného souboru.

Nemíchám je do buildu, který mění vykreslovač — ať víme, co za co může.

---

## CO TESTOVAT

**1)** PS1 bez disku — menu BIOSu, na výšku i na šířku
**2)** Vrať se šipkou do **hlavní nabídky aplikace**
**3)** **Formule** — ta, co měla kostičky
**4)** **Star Wars** — ty, co měly artefakty
**5)** **Medal of Honor 2** — ten, co jel dobře (kontrola, že jsem nic nerozbil)

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz jako dosud | horší než v B82 |
| 2 | **žádný obraz PS1** nikde v aplikaci | prosvítá bootovací Sony |
| 3 | **kostičky pryč** | pořád kostičkované |
| 4 | **artefakty pryč** | pořád artefakty |
| 5 | jako dosud, čistý zvuk | zhoršilo se |

Kroky 3 a 4 jsou ten hlavní důvod tohohle buildu.

## CO POSLAT ZPĚT

Očima podle tabulky. Z logu jeden řádek, ten mi potvrdí, že běží ten
prověřený vykreslovač:

    VYKRESLOVAC: gpu_neon (provereny, soucast jadra PCSX-ReARMed)
