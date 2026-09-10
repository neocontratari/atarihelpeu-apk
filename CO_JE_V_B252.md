# B252 — Atari C++ konečně BĚŽÍ, ne jen testuje (versionCode 298)

Rene: "nehodlám testovat POKEY, kde si já myslím, že to není správné.
Ty sice v logu vidíš, že ti to údajně funguje, ale já nemám podle
zvuku a času toto otestovat. Zruš pomocná tlačítka zvuku a grafiky.
Po bootování naběhne okamžitě reálné Atari s opravdovým snímkováním.
Po BYE naběhne self test - a tam v testu zvuku poběží zvuk přesně tak
jak na reálném Atari. Tam já uvidím, jestli to šlape či nikoli."

Zásadní bod: přestal jsem stavět TESTOVACÍ NÁSTROJ (tlačítka, čísla v
logu) a postavil jsem SKUTEČNOU VĚC - appka teď prostě BĚŽÍ, přesně
tak, jak by běžela na tvém reálném stroji vedle tebe. Posoudíš to
uchem a okem, ne z mých čísel.

===============================================================================
 CO ZMIZELO
===============================================================================

- Tlačítko "▶ SPUSTIT ŽIVĚ" (ruční přepínání průběžného vykreslování)
- Tlačítko "🔊 PŘEHRÁT ZVUK" (jednorázový 2vteřinový snímek na požádání)
- Tlačítko "+2 s (jeden krok)" (ruční posun)

Zůstávají: NABOOTOVAT, BYE, SELECT, START, OPTION, HELP - to jsou
skutečné akce, které bys udělal i na opravdovém stroji (zapnutí,
napsání příkazu, stisk klávesy), ne testovací pomůcky.

===============================================================================
 CO PŘIBYLO MÍSTO TOHO
===============================================================================

**Jedna souvislá smyčka**, která se sama spustí hned po kliknutí na
NABOOTOVAT - žádné další tlačítko není potřeba. Běží přesným tempem
skutečného PAL Atari (50 snímků za vteřinu), hlídaným podle
SKUTEČNÉHO uplynulého času (ne pevným intervalem) - takže neběží ani
rychleji, ani pomaleji, než by běžel opravdový stroj.

Obraz (1 snímek) a zvuk (odpovídající počet vzorků) se posouvají
SPOLU, snímek po snímku - zůstávají synchronizované přesně tak, jak
by byly na skutečném stroji. Když appka na chvíli zůstane pozadu
(prohlížeč zadrhne, telefon se na chvíli zamkne), nesnaží se to
"dohnat" donekonecna - je tam strop na 10 snímků najednou.

Zvuk hraje BEZE ŠVŮ - jednotlivé kousíčky zvuku se plánují přesně
tak, aby na sebe navazovaly (žádné cvakání/mezery mezi nimi).

===============================================================================
 ZMĚNA NA NATIVNÍ STRANĚ
===============================================================================

Starý `audioNative()` (jednorázový 2vteřinový snímek, s vynulováním
stavu při každém volání) je pryč - nahrazen `audioChunkNative()`,
který generuje malé kousky zvuku (odpovídající přesně 1 obrazovému
snímku) a **NIKDY nevynuluje stav generátoru** mezi voláními -
navazuje přesně tam, kde skončil předchozí kousek, přesně jak to
dělá skutečný POKEY čip. Vynulování dávalo smysl jen pro starý
"nezávislý snímek na požádání" - ne pro opravdové průběžné hraní.

Přidána i lehká `regsNative()` pro řídké logování (jednou za ~100
volání, cca 2 vteřiny) - log pořád ukazuje aktuální stav registrů,
jen ne při každém jednotlivém volání (to by ho při 50 voláních za
vteřinu okamžitě zahltilo).

===============================================================================
 OVĚŘENÍ
===============================================================================

Celý `nap_atari_native.cpp` znovu zkompilován čistě s JNI stub
hlavičkou. Izolovaný POKEY test (6 kontrol) spuštěn znovu - beze
změny prošel, protože samotný algoritmus generování zvuku se
nezměnil, jen KDY a JAK ČASTO se volá.

Nová jsdom simulace (`test_atari_cpp_smycka.js`), čtyři kontroly:
  - stará pomocná tlačítka (ŽIVĚ, ZVUK, KROK) v HTML už neexistují
  - klik na NABOOTOVAT sám od sebe spustí smyčku, žádné další tlačítko
  - smyčka volá obraz i zvuk společně, snímek po snímku
  - tlačítka pro skutečné akce (SELECT, HELP) fungují dál beze změny

CO NEJDE OVĚŘIT ODSUD: jak se to skutečně chová a zní na tvém
telefonu - to je přesně to, co teď budeš moct posoudit sám, uchem a
okem, proti svému reálnému Atari 130XE.
