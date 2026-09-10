# B255 — Skutečný studený start, RESET a RETURN (versionCode 301)

Rene po testu: "Není to správně. Self test zvuk není čistý. Bootovací
zvuk není správný. CSAVE máš asi ve smyčce. Přidej tlačítko reset a
return. Po znovu nabootování po self testu skočí zelená obrazovka.
Atari musí být stoprocentní, jinak se nám to sesype jako domeček z
karet."

===============================================================================
 NEJZÁVAŽNĚJŠÍ NÁLEZ: BOOT NEBYL SKUTEČNÝ STUDENÝ START
===============================================================================

V logu bylo vidět, že druhé bootnutí (po self-testu) skončilo na
úplně jiné adrese procesoru (PC=$C02C) než první boot (PC=$F312), a s
jinou hodnotou PORTB (255 místo 253) - to je registr, který řídí
bankování paměti. To znamená, že OS při druhém bootu **běžel úplně
jinudy**.

Příčina: `Machine::reset()` čistil jen procesor a PIA (kvůli
bankování paměti), ale **ANTIC** (rozpracovaný display list), **GTIA**
(barvy, PRIOR, CONSOL) a **POKEY** (AUDF/AUDC/AUDCTL, celý zvukový
generátor včetně kliku) **zůstávaly z předchozí relace** - self-test,
zaseknuté CSAVE, cokoli, co se dělo předtím. Appka tedy nikdy
neudělala skutečný studený start - druhý a každý další boot jel dál s
cizím, nesouvisejícím stavem po sobě.

**Přesně tohle je ta "zelená obrazovka" po druhém bootu.**

===============================================================================
 OPRAVA
===============================================================================

Místo ručního vyjmenovávání KAŽDÉHO pole, které se má vyčistit
(riziko, že se příště na něco zapomene a chyba se vrátí), appka teď
při BOOT tlačítku **smaže a znovu postaví celý stroj** - přesně jako
vypnutí a zapnutí napájení. Paměť se navíc zvlášť ručně vynuluje
(nemá vlastní výchozí nastavení).

Ověřeno novým testem: "znečištěný" stroj (se zapsanými POKEY/GTIA/
ANTIC registry, simulující stav po self-testu/CSAVE) **vůbec
neovlivní** čerstvě vytvořený stroj - ten je vždy čistý, bez ohledu na
to, co dělal ten předchozí.

===============================================================================
 RESET A RETURN TLAČÍTKA
===============================================================================

**RESET** - skutečné chování Atari RESET tlačítka: **nemaže paměť**
(na rozdíl od BOOT, který teď dělá poctivý studený start) - jen znovu
nahodí procesor. Funguje i jako záchranná brzda, když se ROM někde
zasekne.

**RETURN** - samotná klávesa Enter, bez psaní textu (užitečné třeba
po CSAVE, kdy Atari vyzve "stiskni PLAY+RECORD a RETURN").

===============================================================================
 LEPŠÍ VIDITELNOST KLIKU REPRODUKTORU
===============================================================================

Log dosud ukazoval jen AUDF/AUDC/AUDCTL - ale klik reproduktoru
(z minulého buildu) běží přes úplně jiná pole, takže z logu nešlo
poznat, jestli se kliky vůbec spouští. Přidán stav bitu reproduktoru
a počítadlo kliknutí do `regsNative()` - příští log už ukáže, jestli
k pokusu o klik vůbec došlo.

===============================================================================
 CO ZATÍM NEVYŘEŠENO
===============================================================================

**CSAVE zaseknutí** - v logu bylo vidět, že po napsání CLOAD zůstaly
registry AUDF/AUDC/AUDCTL beze změny přes 16 vteřin v kuse, což
ukazuje na uvíznutí (možná čekání na kazetový port, který appka
zatím neemuluje). Tohle zatím není vyřešené - potřebuje další
diagnostiku. RESET tlačítko teď alespoň dává způsob, jak se z
takového stavu dostat během testování, místo aby ses musel spoléhat
jen na nový boot.

===============================================================================
 OVĚŘENÍ
===============================================================================

Celkem 9 izolovaných C++ testů (8 předchozích + nový na studený
start) - všechny prošly. Celý `nap_atari_native.cpp` znovu
zkompilován čistě. Čtyři jsdom simulace (smyčka, zvukový buffer,
příkazy, RESET/RETURN) - žádné selhání nikde (explicitně
prohledáno na "false"/"CHYBA"/"NESEDI" v celém výstupu).
