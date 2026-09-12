# B264 — Poslechl jsem tvou radu (versionCode 310)

Rene: "To Atari a POKEY a logika ti evidentně nejde... podívej se do
kódu Atari emu 130XE VBXE JAVA - tam je návod. Nehodlám neustále
testovat ty samé chyby dokola."

Měl jsi pravdu. Šel jsem tentokrát nejdřív důkladně přečíst tvou
fungující referenci, ne dál rozebírat ROM po instrukcích od nuly.

===============================================================================
 1) OPRAVIL JSEM VLASTNÍ CHYBU Z MINULA
===============================================================================

Moje poslední "oprava" SKSTAT (přidání náhodného šumu na bit 4) byla
**špatně**. V referenčním kódu je přímo napsaný komentář, který
popisuje přesně tenhle přístup jako už dřív vyzkoušený a **vyloženě
zavržený**: "žádný fake CLOAD... vlastní CSAVE nepouští datový chaos
do SKSTAT."

Skutečná reference ukazuje: bit 4 zůstává na výchozí hodnotě, pokud
není aktivní opravdový kazetový tón/signál A neběží motor. Přesně to,
co appka dělala PŘED mou chybnou opravou. Vrátil jsem to zpátky.

Zároveň jsem porovnal svou implementaci časovačů (z minulého kroku)
řádek po řádku s referencí - **tam žádná chyba nebyla**, sedí přesně.

===============================================================================
 2) NOVÝ, SKUTEČNÝ VÝKONOVÝ NÁLEZ
===============================================================================

V logu bylo vidět až 4,2 vteřiny výpadku zvuku najednou a stovky
výpadků během pár minut testování. To není drobné zaškobrtnutí - to
je vážná, systematická věc.

Příčina: appka má "dohánějící" smyčku - když na chvíli zaostane, umí
v jednom kroku dohnat až 10 snímků najednou. Ale pro **každý** z
těchto snímků volala tu nejdražší funkci (převod obrázku přes JSON a
base64) - i když se stejně nakonec vykreslí jen ten **poslední**
snímek z celé dávky! Čím víc appka zaostávala, tím víc zbytečné práce
dělala, což ji zpomalovalo ještě víc - přesný bludný kruh.

Přidal jsem levnou funkci, která jen posune snímek dopředu, bez té
drahé práce okolo obrázku - používá se pro všechny snímky KROMĚ
posledního v dávce. Zvuk se přitom generuje pro úplně každý snímek
(nic se nepřeskakuje) - jen ten drahý obrázek se počítá jen jednou.

===============================================================================
 OVĚŘENÍ
===============================================================================

Nová jsdom simulace potvrzuje: drahá funkce se teď volá přesně
jednou za tik, bez ohledu na to, kolik snímků se dohání. Základní
tlačítka (boot, select, reset, CSAVE) fungují beze změny. Celý
`nap_atari_native.cpp` znovu zkompilován čistě po vrácení SKSTAT.
