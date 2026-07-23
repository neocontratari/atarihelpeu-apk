# KROK L — kořen zpoždění zvuku NALEZEN

## Tvoje poznámka všechno změnila

Řekl jsi, že zpoždění je **i na mobilu** a **i u Segy**. Tím jsi vyloučil
přenos, wifi i prohlížeč — ty s tím nemají nic společného. Muselo to být
uvnitř aplikace, ve společné cestě zvuku. A tam to taky je.

## Co jsem našel

V nastavení zvuku je pravidlo: když je telefon s **Androidem 9 nebo
Galaxy S8 — přesně tvůj případ** — dostane zvuk zásobník **8192 vzorků**.
To je při 44 100 Hz zhruba **186 ms zvuku dopředu**, a k tomu ještě
trojnásobek systémového minima.

Ta proměnná se dokonce jmenuje `s8NoStarve` — "aby S8 nehladověl".

**Není to chyba, je to záměrné rozhodnutí někoho přede mnou.** Zvuk kdysi
praskal a vypadával, tak dostal velký zásobník, aby měl vždycky z čeho
brát. Cena je to zpoždění, které slyšíš. Vyměnilo se "praská" za
"opožďuje se".

## Co jsem udělal

Zmenšil jsem zásobník **na polovinu** (8192 → 4096 vzorků), u PS1
i u Segy. Zpoždění by mělo klesnout ze ~186 ms na ~93 ms.
Do logu se navíc vypíše přesné číslo:
`L zvuk PS1: zasobnik 4096 vzorku = 92 ms zpozdeni`

## POZOR — tohle je výměna, ne oprava zadarmo

Menší zásobník = menší zpoždění, ale **větší riziko, že zvuk začne
praskat nebo vypadávat**. Přesně proto tam ta velká hodnota byla.

Proto tě prosím: **poslouchej pozorně, jestli zvuk nepraská.**
- Když je čistý a míň opožděný → zkusíme zmenšit ještě víc (2048).
- Když začne praskat → vrátíme se a najdeme střed.

Je to ladění, ne jednorázová oprava. Ale poprvé víme, kde ten regulátor je.

## Je v tom i krok K (ladění zvuku na TV šipkami)

- **Šipka nahoru** = zvuk se o 50 ms opozdí
- **Šipka dolů** = zvuk se o 50 ms zrychlí
- **Klávesa 0** = výchozí

Ukáže se zelený nápis `ZVUK ... ms`. Šipky na klávesnici počítače,
v prohlížeči s obrazem.

## Verze: EMU10-L-ZVUK-BLIZ

## Co mi pošli

1. **Je zvuk na MOBILU míň opožděný?** (tohle je ten hlavní test)
2. **Nepraská?** Nevypadává?
3. Řádek z logu `L zvuk PS1: ...`
4. Na TV: jaké číslo ti sedlo šipkami?
