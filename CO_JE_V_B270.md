# B270 — Skutečná příčina "žádný zvuk" na telefonu: drift v čítání času

Díky za testování na skutečném zařízení — bez toho bych tohle nikdy
nenašel, protože v mém CLI testu appka běží v dokonale pravidelném
tempu, jaké reálný telefon nikdy nemá.

===============================================================================
 CO SES POTVRDIL, ŽE FUNGUJE (dobrá zpráva)
===============================================================================

**CLOAD → ERROR 138 logika je správně** — jen jsi musel počkat déle
než 54 vteřin, protože appka na tvém telefonu neběží přesně stejným
tempem jako v mém testovacím prostředí. Kurzor se i správně posunul
o řádek.

===============================================================================
 SKUTEČNÁ PŘÍČINA "ŽÁDNÝ ZVUK"
===============================================================================

Moje oprava z minula (zachytávání přesných přechodů reproduktoru)
počítala, NA KTEROU POZICI v bufferu jednotlivé přechody patří,
podle **odhadu**: "kolik cyklů procesoru uplynulo od posledního
volání". Tenhle odhad se počítal tak, že se pokaždé jen PŘIČETLO
"kolik vzorků jsem právě vygeneroval krát kolik cyklů na vzorek".

V mém testu to fungovalo dokonale, protože jsem volal generování
zvuku PŘESNĚ 882 vzorků PŘESNĚ jednou za každý snímek — naprosto
pravidelně.

**Na skutečném telefonu appka takhle pravidelně neběží.** Prohlížeč
volá generování zvuku podle toho, kolik reálného času skutečně
uplynulo (kvůli tomu, jak funguje přehrávání zvuku) — ne podle
pevného počtu snímků. Drobné odchylky se s každým voláním sčítaly,
až se ten odhadovaný "kde jsme v čase" rozjel od skutečnosti natolik,
že se zaznamenané přechody reproduktoru propočítaly mimo aktuální
buffer a bezpečnostní pojistka je všechny narazila na okraj — místo
skutečného tónu buď nic, nebo nesmyslné škrábnutí na kraji.

===============================================================================
 OPRAVA
===============================================================================

Žádné sčítání odhadů. Pozice v bufferu se teď počítá **vždy znovu**
přímo z aktuálního stavu procesoru — jediného čísla, které je vždy
spolehlivě správné, ať appka běží jakkoliv nepravidelně.

===============================================================================
 OVĚŘENÍ
===============================================================================

Přidal jsem nový test, co přímo simuluje nepravidelné časování jako
na skutečném telefonu (různé počty snímků najednou, různé velikosti
zvukového bufferu místo pevných 882) — zvuk se teď správně objevuje
ve všech sledovaných okamžicích, ne jen náhodně.

Všech 10 předchozích testů prochází (jeden musel být upraven, protože
simuloval nereálnou situaci, která v novém, správném modelu funguje
jinak, ale správně).

===============================================================================
 POCTIVĚ: CO TOHLE NEŘEŠÍ
===============================================================================

- **Self-test zvuk/rychlost** — samostatná výkonnostní otázka, ještě
  nezkoumaná.
- **CSAVE chybějící závěrečná "úvodní stopa"** (ten typický zvuk na
  konci, co je na skutečném Atari) — ještě nezkoumáno.
- **Attract mode (změna barev) v praxi na telefonu nefunguje**, i
  když můj izolovaný test ukázal správné chování po ~11 minutách —
  pravděpodobně souvisí se stejnou výkonnostní otázkou (pokud appka
  běží pomaleji než 50 snímků/s na tvém telefonu, těch 11 minut
  "atari času" trvá v reálném čase podstatně déle).

Všechny tři zůstávají pro příští kolo.
