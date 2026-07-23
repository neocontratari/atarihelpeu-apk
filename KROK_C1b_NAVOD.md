# KROK C1b — opraveno: obraz běží přímo v aplikaci

Parťáku, díky za zpětnou vazbu — obojí opraveno.

## Co jsem opravil

**1) Tlačítko ti zakrývalo TV Viewer a options.**
Přesunul jsem ho na tvoje logo NaP (vlevo nahoře), je průhledné a bez
nápisu — jen neviditelná klikací plocha přes logo. Všechno tvoje
ovládání se ti tím vrátí.

**2) Hra se zasekla — našel jsem přesnou příčinu.**
Nebyla to chyba obrazu. Tvoje aplikace má pravidlo: když jde do pozadí,
natvrdo zastaví běžící PS1 hru (`stopPs1SessionHard` v onPause).
Náš obraz se ale otevíral v NOVÉM okně → hlavní obrazovka šla do pozadí
→ hra se sama vypnula → nebylo co kreslit → zaseknutí.

Řešení: obraz už nové okno neotevírá. Kreslí se **přímo v tvé hlavní
obrazovce**, navrch přes web. Hra tím pádem běží dál. Navíc je to blíž
tomu, co budeme dělat v kroku C2 (výměna na místě).

## Jak to vyzkoušet

1. Rozbal ZIP → zkopíruj CELÝ obsah přes repozitář → **Nahradit vše**.
   Commit „krok C1b" → Push → počkej na zelenou → APK do mobilu.
2. Otevři aplikaci (verze EMU10-C1b).
3. **Nejdřív spusť PS1 hru** jako obvykle, nech ji běžet.
4. **Klepni na svoje logo NaP vlevo nahoře** → obraz se přepne na náš
   plynulý OpenGL.
5. **Dalším klepnutím na stejné místo se vrátíš zpět** (je to přepínač).

## Co mi pošli

1) Byl build zelený?
2) Co vidíš po klepnutí na logo — je tam obraz hry? Běží plynule?
   Barvy? Otočení? Nezasekne se teď už hra?
3) Log z 8765/log — hledám řádky „C1" (options už ti tlačítko
   nezakrývá, tak by to mělo jít poslat).

Kdyby byly barvy divné nebo obraz vzhůru nohama — nevadí, napiš mi to
a doladím. Jsme kousek od cíle.
