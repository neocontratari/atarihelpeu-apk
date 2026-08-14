# B98 — POŘADÍ STARTU ENKODÉRU (versionCode 146)

> **Tento kód je předpoklad. Čeká se na test na telefonu.**

## Ten log ukázal příčinu přesně

Dva řádky za sebou:

    23:53:51.547  TV_PRIMO_ZAPNUTO       <- predam okno do C a zacnu kreslit
    23:53:51.552  H264_ENCODER_START     <- ...a TEPRVE TED se enkoder spousti

**Předával jsem okno enkodéru dřív, než se enkodér spustil.** Nativní vlákno
začalo kreslit do enkodéru, který ještě neběžel — a to shodí celou aplikaci.

A sedí to i s tím, že v logu **není ani jeden z mých kroků KROK1 až KROK4** —
spadlo to ještě předtím, než vlákno vůbec začalo.

Bylo to v mém kódu z B94 a přežilo to tři buildy, protože jsem hledal chyby
uvnitř toho vlákna místo toho, kdy se spouští.

## Opravy

**1. Pořadí.** Okno se předá až potom, co enkodér opravdu běží.

**2. Pojistka.** Nativní vlákno navíc počká, až bude mít okno enkodéru
rozumnou velikost (až vteřinu). Když ji nedostane, **skončí a nekreslí** —
místo aby shodilo aplikaci. V logu `TV_PRIMO KROK5_OKNO_BEZ_VELIKOSTI`.

## Průběžný zápis logu vypnutý

Zapisoval každou dávku na sdílené úložiště a brzdil systém — máš pravdu.
Vypnuto. Do `Downloads` se teď zapisuje **jen při pádu**, protože to je
jediná chvíle, kdy se log jinak nedá získat.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Zapni **TV web**
**2)** Vypni a zase zapni

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | aplikace běží dál, na TV obraz | spadne |
| 2 | přežije opakovaně | spadne napodruhé |

## CO POSLAT ZPĚT

Jen jestli spadne, nebo ne. Nic víc po tobě nechci.
