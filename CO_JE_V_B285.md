# Co je v B285

Navazuje na B284 a tvoji zprávu: "Furt zahlcuje. Při nastartování apky
self test funguje bez problémů bez lagů. Ale po csave se to postupně
začne zase zahlcovat a po dalším spuštění self testu se začne totálně
rozjíždět zvuk od grafiky... Projeď si kód - projeď si internet a
hlavně pamatuj toto emu atari čistě c++."

## Co jsem nejdřív ověřil (ne odhadl)

Vzal jsem tě za slovo a zaměřil se na C++ jádro: napsal jsem nový test,
který změří **skutečný** čas (na hodinách počítače, ne emulovaný čas
Atari), jak dlouho jádru trvá odbavit 2000 snímků *před* jakýmkoli
CSAVE a znovu *po* třech CSAVE za sebou. Pokud by jádro bylo
"zahlcené", druhé měření by bylo pomalejší.

Výsledek: poměr 1,005–1,015× (opakovaně měřeno) – to je v mezích
běžného šumu měření. **Jádro samo po CSAVE není pomalejší.** Taky jsem
prošel celý soubor jádra a potvrdil, že tam nejsou žádné dynamicky
rostoucí datové struktury (seznamy, mapy apod.), do kterých by se
mohlo "nahromadit" cokoliv.

## Kde to skutečně bylo

V přehrávání zvuku v JavaScriptu, ne v C++ jádru. Tlačítka SELECT/
START/OPTION/HELP (přesně ta, kterými se ovládá self-test) od B284
posílají zvuk jako jeden kus (~0,84 s) najednou. Fronta, která říká
"kdy se má přehrát další kousek zvuku", ale neměla žádný strop – každé
stisknutí ji posunulo o dalších ~0,84 s dál do budoucnosti, i když
mezitím uplynul sotva zlomek vteřiny skutečného času.

Obraz se ale pořád kreslí okamžitě, nečeká na frontu zvuku vůbec. Takže
po pár stiscích při procházení self-testu už bylo naplánováno několik
vteřin zvuku dopředu – to je přesně to, co jsi popsal jako "zvuk ujíždí
od obrazu". A čím delší test (víc stisknutí), tím víc se to
"zahlcovalo", protože fronta jen rostla a nikdy se sama nezkrátila.

CSAVE samo o sobě žádnou frontu nehromadí (přehrává se plynule,
snímek po snímku) – jen muselo "spolknout" frontu, která se nahromadila
z mačkání tlačítek kolem něj.

## Oprava

Fronta zvuku teď má strop (1,2 s – víc, než kolik zabere jedno
stisknutí tlačítka, aby se samo nikdy neoříznulo). Když se strop
překročí, zvuk, co ještě nezačal hrát a je za tím stropem, se zastaví a
fronta se "zkrátí" zpátky blízko k reálnému času – místo aby rostla bez
konce.

Do jádra (C++) ani do Java/JNI vrstvy jsem tentokrát vůbec nesahal –
celá oprava je jen v přehrávání zvuku (index.html).

## Co jsem ověřil a co ještě ne

Ověřil jsem logiku opravy v izolovaném testu (simuluje mačkání tlačítek
s falešnými hodinami, bez skutečného zvuku): bez opravy fronta po 20
stiscích naroste na 10,8 s zpoždění; se opravou zůstane pod 1,1 s;
běžné pomalé používání (jedno stisknutí za 2 s) opravou vůbec není
dotčené; jedno jediné stisknutí se samo nikdy neoříznuje. Celá
existující sada C++ testů (23/23) prochází beze změny.

Co ještě nevím: jak to **zní**, když se fronta ořízne – čekám jemné
"cvaknutí" při dlouhém testu plném rychlého mačkání (to je v pořádku,
mnohem lepší než rostoucí zpoždění), ale jestli to zní rušivěji, dej
vědět a vyhladím to jemněji. Hlavní otázka na tebe: **zmizelo
"zahlcování" a rozjíždění zvuku od grafiky po CSAVE a delším
self-testu?**
