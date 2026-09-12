# B262 — Skutečná disassemblace Atari ROM (versionCode 308)

Rene: "Zvuk je lepší ale furt se kouše. Po CSAVE neustále ten stejný
problém - zasekla smyčka a pak to skočí do ready - ale tak Atari
nefunguje. Opravdu si projdi pořádně core a biosy Atari."

Vzal jsem to vážně - napsal jsem si vlastní jednoduchý disassembler
6502 kódu a prošel jsem SKUTEČNÝ ROM kód, ne jen odhady z hodnot
registrů.

===============================================================================
 CO JSEM NAŠEL - ROZEBRÁNO NA DVĚ FÁZE
===============================================================================

**Fáze 1 (legitimní, správná):** Po napsání CSAVE+RETURN+RETURN ROM
nastaví dva zvukové kanály a pak čeká na **softwarový časovač
odpočítávaný přerušením VBLANK** (standardní systémová proměnná OS,
$022A) - 255 snímků, tedy zhruba 5 vteřin. **Tohle je správné,
očekávané chování** - to je přesně ta "příprava", o které jsi mluvil.
Potvrdil jsem testem, že tahle část funguje správně a sama od sebe
doběhne.

**Fáze 2 (hlubší, dosud neopravená chyba):** Až PO téhle přípravné
pauze následuje DALŠÍ, hlubší rutina, která už opravdu čte registr
SKSTAT - to je skutečný POKEY obvod pro detekci signálu z kazetového
vstupu. Čeká na PŘECHODY (změny) v jednom konkrétním bitu. Appka ale
předtím vracela **vždy přesně stejnou hodnotu** - dokonalé, umělé
ticho bez jediného přechodu.

Na skutečném hardwaru by NEZAPOJENÝ kazetový vstup nebyl "dokonale
tichý" - byl by na něm elektrický šum (plovoucí, nezapojený vstup),
takže by ROM touhle smyčkou prošla rychle (stačí pár přechodů). Bez
šumu ROM nikdy žádný přechod nevidí.

===============================================================================
 OPRAVA
===============================================================================

SKSTAT bit 4 teď simuluje tenhle šum (stejným generátorem, jaký už
appka používá pro RANDOM registr) - ostatní bity zůstávají beze
změny.

===============================================================================
 UPŘÍMNĚ - CO JEŠTĚ NENÍ VYŘEŠENÉ
===============================================================================

Tahle oprava cílí na tu HLUBŠÍ, POZDĚJŠÍ fázi - tu, kam se v
momentálně testovaném okně (5 vteřin od druhého RETURN) appka ještě
ani nedostane. Takže:

- Ta legitimní 5vteřinová pauza na $FD6C je v pořádku - to není bug.
- **Pořád nevím, proč BĚHEM téhle (správné, očekávané) pauzy appka
  hraje nepřetržitý špatný zvuk** (všechny 4 kanály), místo krátkého
  pípnutí. Potřebuju najít, KDO nastavuje hlasitost (AUDC) pro tyhle
  kanály a proč neexistuje nebo nefunguje mechanismus, který by je
  po krátké době umlčel.

To je přesný další krok, na kterém budu pokračovat.

===============================================================================
 OVĚŘENÍ
===============================================================================

Vlastní disassembler (Python, ~150 řádků) použitý k přečtení
skutečného ROM kódu na adresách $FC80-$EDE0. Všech 6 existujících
testů prošlo beze změny. Celý `nap_atari_native.cpp` znovu
zkompilován čistě.
