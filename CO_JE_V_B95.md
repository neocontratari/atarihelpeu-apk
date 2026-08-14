# B95 — TV BEZ JAVY, TENTOKRÁT BEZ PÁDU (versionCode 143)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Proč B94 spadl při zapnutí TV

Moje chyba a je to učebnicová. Funkce, která podává snímek, vypadala takhle:

```cpp
std::lock_guard<std::mutex> lk(g_frame_swap);   // zamek plati do konce funkce
return g_frame_buf[idx].data();                 // ...ale ukazatel se pouziva AZ POTOM
```

Vrací **ukazatel** do sdílené paměti a zámek pustí dřív, než ho volající
použije.

Dokud snímek četl jen mobil, prošlo to — bylo to jedno vlákno a trefovalo se
mimo. Jakmile se přidala TV, **čtou dva** a vlákno emulace mezitím tu paměť
zvětší (`resize`). Starý ukazatel pak míří do uvolněné paměti → pád.

Proto to padalo **přesně ve chvíli zapnutí TV** a bez TV běželo.

## Oprava

Nová funkce `nap_ps1_kopiruj_snimek()` snímek **pod zámkem zkopíruje** do
paměti volajícího. Každé vlákno pak pracuje se svým.

Je to jedna kopie navíc — ale mezi vlákny se to jinak sdílet nedá a pád je
horší než kopie. Pořád je to o tři kopie míň než javová cesta v B92.

Zapsal jsem to do předávacího balíčku jako past, na kterou si dát pozor.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

Co ověřit nedokážu: chování dvou vláken na skutečném telefonu.

---

## CO TESTOVAT

**1)** Hra na mobilu **bez TV** — kontrola, že je to jako B92
**2)** **Zapni TV** — hlavně jestli aplikace nespadne
**3)** Nech hru chvíli běžet se zapnutou TV
**4)** Vypni TV a zase zapni

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | jako B92 | zhoršilo se |
| 2 | **aplikace běží dál**, na TV obraz | spadne |
| 3 | obraz i zvuk plynulé na obou | kouše se |
| 4 | přežije to opakovaně | spadne napodruhé |

Krok 2 je ten hlavní. Krok 4 taky — opakované zapnutí odhalí, jestli se
vlákna korektně ukončují.

## CO POSLAT ZPĚT

Hlavně: **spadne to při zapnutí TV, nebo ne?**

Z logu, pokud to přežije:

    TV_PRIMO_ZAPNUTO
    TV_PRIMO PRIPRAVENO
    TV_PRIMO: snimku=...
