# B258 — CSAVE: skutečná příčina, ne kazetový port (versionCode 304)

Rene: "CSAVE není kazetový port - Atari i bez kazeťáku po napsání
CSAVE/CLOAD a zmáčknutí RETURN udělá dva krátké tóny, to je příprava,
nemá to se zapojeným kazeťákem nic společného. A navíc tohle bylo
vyřešeno v Java emu Atari. Ale nekopíruj, udělej to přesně podle
Atari jádra."

Měl jsi pravdu a moje dřívější domněnka byla špatná. Šel jsem to
vyšetřit place.

===============================================================================
 JAK JSEM TO ZJISTIL
===============================================================================

Napsal jsem test, který zreprodukoval přesně tvou sekvenci (boot →
napsat CSAVE → počkat → RETURN znovu) a sledoval procesor krok po
kroku. Klíčové zjištění: **procesor se vůbec nezasekává** (JAM zůstává
`false` po celou dobu) - uvízne v naprosto legitimní smyčce OS kódu na
adrese `$FD6C`, která zní (v assembleru) "načti hodnotu z adresy `$11`,
pokud je nula, skoč zpátky a zkus to znovu" - typický vzorec "čekej,
dokud se něco nezmění".

To "něco" se mělo změnit **přerušením od POKEY** - a přesně to, cos
říkal: **nemá to nic společného s fyzickým kazeťákem.** Je to čistě
vnitřní mechanismus čipu.

===============================================================================
 DVA CHYBĚJÍCÍ KUSY SKUTEČNÉHO POKEY - OBA PODLE TVÉ REFERENCE
===============================================================================

**1) Časovače 1/2/4 vůbec neexistovaly.** POKEY má tři vnitřní
odpočítávací časovače, které při dosažení nuly vyvolají přerušení.
Předtím appka tohle vůbec neměla - `STIMER` (příkaz "začni znovu
počítat") jen resetoval zvukové kanály, ne tyhle časovače. Přidal
jsem přesný překlad výpočtu periody a odpočtu z tvé fungující JS
reference - volá se jednou za každý řádek obrazu, přesně jak to
dělá skutečný hardware.

**2) Chybějící "jsem připraven" signál.** Když program povolí
přerušení "sériový výstup potřebuje další bajt" A sériový port je
zrovna v klidu (nic se neposílá), skutečný POKEY **okamžitě**
odpoví "jsem připraven, dej mi bajt" - i bez toho, aby cokoli bylo
předtím posláno. Appka tohle dřív signalizovala **jen po prvním
odeslaném bajtu** - takže OS čekal na povolení, které nikdy
nepřišlo, protože ono samo čekalo na tenhle první signál. Slepá
ulička. Opraveno.

===============================================================================
 OVĚŘENÍ
===============================================================================

Před opravou: PC zamrzlé na `$FD6C`, beze změny přes 2000+ snímků v
kuse (odpovídá tvému hlášení "16 vteřin v kuse" v logu).

Po opravě: PC se aktivně mění, žádné trvalé zaseknutí.

Nový konsolidovaný test, šest kontrol:
- boot dává stejný, správný výsledek jako dřív (přesně ten kritický
  test "dva nezávislé starty dají identický výsledek" z minulé
  regrese - nechtěl jsem touhle hlubokou opravou nic pokazit)
- POKEY tón zůstává přesný (0,4 % odchylka, stejně jako dřív)
- JAM pořád vede na ticho
- STIMER teď správně nabíjí i IRQ časovače, ne jen zvuková čítadla
- povolení sériového přerušání v klidu okamžitě signalizuje
  připravenost

Celý `nap_atari_native.cpp` znovu zkompilován čistě. Žádné Java
změny - celá oprava je v jednom C++ souboru (`nap_atari_machine.h`).

===============================================================================
 CO TO ZNAMENÁ PRO TEN DVOUTÓNOVÝ SIGNÁL
===============================================================================

Tahle oprava odstraňuje zaseknutí, které bránilo OS kódu pokračovat
dál po CSAVE/CLOAD. Jestli teď appka skutečně přehraje ten
charakteristický dvoutónový signál přesně tak, jak má, to už je na
tvém testu - ale cesta k tomu teď není zablokovaná.
