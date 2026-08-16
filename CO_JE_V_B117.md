# B117 — SEGA PŘES GPU, STEJNĚ JAKO PS1 (versionCode 165)

> **Tento kód je předpoklad. Čeká se na test.**

## Co jsem našel, když jsem si Segu prošel

Nativní jádro `clownmdemu` v projektu **už bylo** a překládalo se. Umělo
nahrát ROM, krokovat, zvuk i vstup — a hlavně **už vyrábělo snímek**,
kreslilo po řádcích do `g_real.frame_argb` v ARGB, přesně jako PS1.

**Chyběla jediná věc: nikdo si ten snímek nebral.** Obraz se místo toho
snímal z okna aplikace — pomalá cesta, odtud to zpoždění proti mobilu.

## Co je hotové

```
jadro clownmdemu (C)
     -> g_real.frame_argb          uz bylo
     -> GL textura                 NOVE
     -> obrazovka                  NOVE
```

Nová část v `nap_sega_native_proof.cpp`:

- vlastní EGL kontext a vlákno, **nic se s jádrem nesdílí**
- snímek se **kopíruje pod zámkem** (na tomhle spadla PS1 v B94)
- formát okna přes `setBuffersGeometry` (bez toho to padá, viz PS1 B96)
- generace vláken, aby staré nekreslilo vedle nového (PS1 B96)
- `surfaceChanged` znovu nepřipojuje (PS1 B78)
- otočení a prohození R/B rovnou správně (PS1 B89)

Vzal jsem si všechna poučení z PS1, aby se stejné chyby neopakovaly.

**Na TV** jde snímek přímo z jádra přes `grabFrame()` — stejné rozhraní
jako PS1, takže se zpracování, ořez i enkodér nemění. Okno aplikace se
kvůli Sege už nesnímá.

**Plocha** leží pod stránkou (ovladač zůstává nad obrazem) a mimo
obrazovku Segy se schovává.

## Z B116 zůstává

Oprava tempa snímání a ovladač: rychlejší dotyk (`touch-action:none`),
klávesnice a joystick přímo do jádra, Xbox mapování.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Sega C++ | `clang` pro aarch64, jako v CMake | 0 chyb |
| PS1 C++ | `clang` pro aarch64 | 0 chyb |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| nativní funkce vs. Java | 1:1 | sedí |
| PS1 symboly | `nm -u` proti `nm --defined-only` | sedí |

---

## CO TESTOVAT

**1)** **Sega na mobilu** — naskočí obraz?
**2)** **Sega na TV** — je zpoždění pryč? Sedí mobil s obrazovkou?
**3)** **PS1** — kontrola, že jsem nic nerozbil
**4)** Vrať se ze Segy do nabídky — neprosvítá její obraz jinde?

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz je, ovladač nad ním | černo |
| 2 | **bez zpoždění**, mobil = obrazovka | pořád delay |
| 3 | jako v B114 | zhoršilo se |
| 4 | žádný obraz Segy mimo její obrazovku | prosvítá |

Když bude krok 1 černý, hledej v logu `SEGA_PLOCHA_VYTVORENA`
a `SEGA_OBRAZ_PRIMO_ZAPNUT` — z toho poznám, kde to vázne.

## Co ještě zbývá

**Zvuk Segy** pořád jde starou cestou. Jádro už ho umí dodat nativně
(`pullAudio`), ale aplikace na to musí přepnout — to je další krok, až
bude obraz potvrzený.
