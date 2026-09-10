# B254 — CSAVE/CLOAD: teď se dá napsat (versionCode 300)

Rene: "po napsání CSAVE nebo CLOAD a odmáčknutí RETURN Atari udělá
specifický potvrzovací zvuk (tu tu), dřív než kazeťák vůbec něco
dělá - kazeťák tam vůbec nehraje roli, je to příkaz, po kterém Atari
zvukem oznamuje, že je připraveno."

===============================================================================
 CO SE ZJISTILO
===============================================================================

Tohle NENÍ o emulaci kazety (čtení/zápis zvukem kódovaných dat na
pásku) - to je samostatná, mnohem větší věc, která teď opravdu chybí.
Tohle je JEDNODUŠŠÍ: BASIC po příkazu CSAVE/CLOAD prostě zapíše pár
tónů do POKEY registrů (přesně stejný druh operace, jaký už funguje v
self-testu) - jako potvrzení "jsem připravené", ještě předtím, než by
se cokoli dělo s páskou.

Zkontroloval jsem `kbcode()` (překlad znaku na skenovací kód
klávesy) - **celá abeceda A-Z tam už byla hotová**, ne jen B, Y, E,
jak jsem si myslel. Ověřil jsem to **programově**, ne okem - vytáhl
jsem skutečnou tabulku z tvé JS reference (`var KEY={...}`) a
porovnal ji se všemi 26 hodnotami v Javě - přesná shoda na celou
abecedu, včetně C, S, A, V, E, L, O, D, které potřebuješ pro
CSAVE/CLOAD.

Jediné, co chybělo: appka uměla napsat jen napevno "BYE" - žádná
možnost napsat cokoli jiného.

===============================================================================
 CO PŘIBYLO
===============================================================================

Obecná funkce `atariNapisText(text)` na Java straně - napíše libovolný
text + RETURN, stejným způsobem jako už fungující "napiš BYE".

Na testovací stránce dvě tlačítka - "NAPSAT: CSAVE" a "NAPSAT: CLOAD"
- a navíc pole pro vlastní příkaz (např. `PRINT 1+1`), pro budoucí
testování.

Žádná změna v generování zvuku ani v mechanismu kliku z minula (B253)
- tohle jen umožňuje dostat se k příkazu, který by měl existující,
už fungující mechanismus (BASIC píše do POKEY/GTIA, appka to přehraje)
ozvučit stejně spolehlivě jako self-test.

===============================================================================
 OVĚŘENÍ
===============================================================================

Rovnováha závorek v MainActivity.java ověřena chytrým kontrolorem - 0.
TV cesta - 0 chyb. Programové porovnání celé abecedy kód-po-kódu s
referencí - přesná shoda.

Nová jsdom simulace (`test_atari_cpp_prikazy.js`), tři kontroly:
  - tlačítko CSAVE zavolá `atariNapisText("CSAVE")`
  - tlačítko CLOAD zavolá `atariNapisText("CLOAD")`
  - vlastní příkaz z textového pole se pošle velkými písmeny a pole
    se po odeslání vyprázdní

CO NEJDE OVĚŘIT ODSUD: jestli BASIC skutečně dojde až k
tónu-generujícímu kódu po CSAVE/CLOAD (vs. jestli by se třeba
zaseklo na čekání na kazetový port dřív, než tón stihne přehrát) - to
uvidíme přímo z tvého testu.
