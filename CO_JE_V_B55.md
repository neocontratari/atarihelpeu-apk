# B55 — TEČKY A KOUSÁNÍ PRYČ (versionCode 103)

Obojí jsem způsobil v B54. Tady jsou obě příčiny a obě opravy.

## 1) Tečky po obraze

V novém formátu obrazu (5551) se **průhlednost ukládá do bitu masky**, což je
nejvyšší bit hodnoty. A ta hodnota se používá jako **číslo řádku v paletě**.
U neprůhledných útvarů byla průhlednost 1,0, takže se ten bit vždycky zapsal
a čtyřbitové textury pak sáhly do palety o osm řádků vedle. Odtud barevné
tečky rozseté po obraze.

Oprava: do obrazu se zapisuje nula, průhlednost se řeší mícháním přes pevnou
hodnotu (`glBlendColor` + `GL_CONSTANT_ALPHA`). Výsledek na obrazovce je
stejný, ale data v paměti už nejsou porušená.

## 2) Kousání zvuku i obrazu

Z tvého logu:

    B53:  kresleni =  97 na snimek
    B54:  kresleni = 265 na snimek

V B54 jsem kopie uvnitř videopaměti přesunul na grafiku. Každá si vynutila
dokreslení **a ke každé se kopíroval celý megabajt** — to je ~170 MB na
snímek navíc.

A hlavně: **k ničemu to nebylo.** Ověřil jsem si u sebe, že obraz je s tou
kopií i bez ní úplně stejný. Takže je pryč a kopie se řeší jako v B53.

## Co z B54 zůstává

Formát obrazu 5551 a texturování z obrazu místo z paměti procesoru. To je ta
věc, kvůli které se koule na pozadí BIOSu kreslí jako koule.

## Jak to poznáš

- **Hra**: kousání i tečky mají zmizet. Zátěž se má vrátit na ~97 kreslení
  na snímek — v logu řádek `NAPLES2 ZATEZ`, položka `kresleni` děleno 120.
- **BIOS bez disku**: zvuk plynulý.

## Kdyby to bylo pořád horší než B53

Nezdržuj se tím a vrať to: v GitHub Desktop v záložce History klikni pravým
na commit s B54/B55 a dej **Revert**. Vrátíš se na B53, který ti běžel.
Řekni mi to a budu pokračovat u sebe, dokud to nebude sedět.

## Co pořád není opravené

Zelená změť přes MEMORY CARD a CD PLAYER v menu BIOSu. Na to jdu dál.
