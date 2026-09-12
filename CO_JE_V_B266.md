# B266 — Vícestopé CD: skutečná chyba nalezena a opravena (versionCode 312)

Rene: "Tato hra je skládaná z několika souborů, ne jen z jednoho.
Mate mě na tom to, že hra Duke Nukem 3D funguje bez problémů, a
myslím, že graficky je na stejném principu."

Tohle je skutečně cenná stopa - a vedla přímo ke konkrétní, jasné
chybě.

===============================================================================
 CO JSEM NAŠEL
===============================================================================

Appka má kód, který sám sestaví `.cue` soubor pro hry, co dorazí jako
víc `.bin` souborů bez vlastního `.cue` (přesně to "_ps1_synth.cue",
co bylo vidět v tvém logu). PS1 Doom má na rozdíl od PC verze
opravdovou hudbu z CD (ne MIDI) - proto má víc zvukových stop. Duke
Nukem 3D pravděpodobně tolik stop nemá, proto funguje bez problémů.

**Skutečná chyba:** stopy se řadily obyčejným abecedním porovnáváním
textu. To znamená, že "track10.bin" se v abecedě řadí PŘED
"track2.bin" (protože znak '1' je před '2') - takže u her s 10 a více
zvukovými stopami by se stopy zařadily do sestaveného `.cue` souboru
ve ŠPATNÉM pořadí. Hra by pak četla nesprávná data z nesprávně
označených stop.

===============================================================================
 OVĚŘENÍ - SKUTEČNĚ SPUŠTĚNO, NE JEN ODHAD
===============================================================================

Napsal jsem "přirozené řazení" (čísla uvnitř názvu souboru se
porovnávají jako čísla, ne jako text) - to je standardní řešení
přesně pro tenhle problém.

Tohle jsem mohl opravdu zkompilovat a spustit (Java, ne jen C++) -
potvrdil jsem: staré řazení SKUTEČNĚ selhávalo přesně tak, jak jsem
předpokládal (track10 před track2), a nové řazení to opravuje
správně, včetně čísel s nulami na začátku (track01, track02...).

===============================================================================
 DRUHÁ ZMĚNA - S MENŠÍ JISTOTOU, ŘÍKÁM TO ROVNOU
===============================================================================

Přidal jsem taky standardní dvouvteřinovou "pregap" mezeru před
zvukové stopy (běžná součást specifikace CD-DA zvuku). Tady ale
musím být upřímný: záleží na tom, jestli extrahované soubory tuhle
mezeru už samy obsahují, nebo ne - a to bez konkrétních souborů
nemůžu odsud zjistit jistě.

Riziko je ale omezené - nejhorší možný důsledek špatného odhadu je
dvouvteřinový posun nebo drobná chyba ve zvuku, ne poškození dat.

===============================================================================
 CO SE NEZMĚNILO
===============================================================================

Žádný zásah do Atari souborů.

===============================================================================
 DALŠÍ KROK
===============================================================================

Tvůj další test s vícestopou hrou (Doom) ukáže, jestli pořadí stop
(případně pregap) byla skutečná příčina té rozsypané grafiky, nebo
jen část problému.
