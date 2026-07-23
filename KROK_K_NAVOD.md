# KROK K — zvuk přestavěný od základu

## Proč tvoje testy 0,5 a 0,9 nefungovaly — našel jsem to

V kódu, který přehrává zvuk v prohlížeči, byla funkce, co po každém
zaškobrtnutí zvuk přeskládá. A ta měla **natvrdo napsáno
`next = t + 0.36`** — tedy si vždycky nastavila zpoždění 0,36 vteřiny,
**bez ohledu na to, co jsme nastavili my**.

Protože se přeskládávalo často (při každé mezeře v datech), tvoje
nastavení se nikdy neudrželo. **Ta cesta nebyla špatná — jen ji něco
pořád přepisovalo.** Měl jsi pravdu, že se to musí vzít z gruntu.

## Co je přestavěné

1. **Jedna hodnota zpoždění pro všechno.** Přeskládání ji už nepřepisuje,
   drží se.
2. **Přeskládává se jen při skutečném výpadku**, ne při každé mezeře.
   Dřív to střílelo pořád a rozbíjelo časování.
3. **Zvuk si hodnotu pamatuje** — jednou naladíš, zůstane i po zavření
   prohlížeče.

## A teď to hlavní pro tebe: ladíš to ZA BĚHU, přímo u hry

Na stránce s obrazem:

- **Šipka nahoru** = zvuk se o 50 ms opozdí (když předbíhá)
- **Šipka dolů** = zvuk se o 50 ms zrychlí (když se opožďuje)
- **Klávesa 0** = zpátky na výchozích 300 ms

Při každém stisku se nahoře krátce ukáže zelený nápis `ZVUK 350 ms`,
ať víš, kde jsi. Nemusíš nic přenačítat ani nic psát do adresy —
koukáš na pusu postavy a šipkami se trefuješ, dokud to nesedne.

## Verze: EMU10-K-ZVUK-OD-ZAKLADU

## Co mi pošli

1. Podařilo se ti šipkami zvuk trefit?
2. **Jaké číslo ti sedlo** (to zelené `ZVUK ... ms`)?
3. **Drží to?** Nebo se to po pár minutách zase rozejde?

Ta třetí otázka je nejdůležitější. Když to drží → hotovo, nastavím to
natrvalo. Když se to postupně rozchází → jde o rozdílné hodiny na obou
stranách a doděláme průběžné dorovnávání.
