# Co je v B287

Navazuje na tvůj opakovaný, a tentokrát ještě důraznější požadavek:
"Ne - tu kontrolu udělej ještě před testem - chci čisté jádro atari
emu v c++ - Java odhaduje a to je problém - Partaku nelákej mě - atari
emu v Javě je v aplikaci. Proto teď děláme atari emu v c++ !!!"

## Co jsem našel, než jsem začal cokoliv měnit

Nejdřív jsem si ověřil, co přesně "atari emu v Javě je v aplikaci"
znamená - a vyšlo najevo něco, co v žádném dřívějším buildu
zaznamenané nebylo: **appka má dvě úplně oddělená Atari jádra.**

1. Staré, kompletní, celé ručně napsané v JavaScriptu (vlastní 6502
   procesor, vlastní zvuk přes Web Audio) - a přesně tohle běží pod
   hlavním tlačítkem menu "ATARI 130XE EMULATOR".
2. Naše C++ jádro, na kterém děláme od B282 - to běží pod tlačítkem
   HELP (v appce nikde ani nevysvětlené, proč tam je).

V gitu má to staré JS jádro přesně jeden commit - nedotčené od
začátku viditelné historie. Tvůj vlastní testovací log to navíc
potvrzuje: hlášku `ZVUK_PREDSTIH_OREZAN`, kterou jsi mi posílal, umí
vypsat JEN C++ jádro - takže jsi celou dobu testoval správně přes
HELP a nic z práce na B282-B286 nebylo nazmar.

Tohle jsem ti nahlásil jako kontrolní bod před pokračováním a tys
rozhodl: hlavní menu / staré JS jádro se prozatím nemění, ale jádro
pod HELP musí být doopravdy, kompletně čisté C++ - žádná JS/Java
"odhadovací" vrstva se v něm nesmí schovávat.

## Co jsem udělal

Poslední taková vrstva byl zvuk: `emu_atari_cpp/index.html` pořád
přehrával zvuk přes JS Web Audio API s ručním plánováním (přesně to,
co jsme spolu ladili v B285 a B286 - proměnná "dalšíZvukStart" a
okolo ní postavená fronta). **To je teď celé pryč** - ne vypnuté,
rovnou smazané (~140 řádků). Zvuk teď hraje přímo z C++ jádra přes
nativní OpenSL ES - stejný, už ověřený a v produkci běžící způsob,
jaký má PS1 jádro téhle appky.

Nové C++ (kruhový buffer + 4× zisk/oříznutí/převod na číslo pro
reproduktor, co dřív dělal JS `GainNode`) jsem schválně napsal v
samostatném souboru bez jakékoli závislosti na Androidu - abych to
mohl pořádně otestovat i na tomhle počítači, ne jen slíbit, že to
bude fungovat. Napojil jsem ho na STEJNÁ tři místa, kde jádro už
zvuk generovalo (žádné nové volání generátoru nepřibylo) - jen se
výsledek navíc posílá do nativní fronty k přehrání. Start/stop zvuku
jsem navázal na otevření/zavření obrazovky HELP stejným způsobem,
jaký už má PS1 (schválně - jednou se v appce stalo, že zvuk jednoho
jádra "utekl" do druhého, tomu se chci vyhnout předem, ne až po
nahlášení).

Nahrávání CSAVE→WAV se nemění vůbec - bere pořád ze stejných bajtů
jako dřív, jen se tyhle bajty navíc (a nezávisle) teď posílají i do
reproduktoru.

## Co jsem ověřil

Nový test (21 kontrol) na kruhovém bufferu a převodu zisku - všechny
prošly: pořadí dat zachováno, zabalení přes konec bufferu funguje
správně, při přetečení se zahazuje nejstarší vzorek (ne nejnovější),
ticho/oříznutí/normální hlasitost sedí na správných hranicích, a
výsledek je shodný s dosavadním přehráváním při násobku 1 (žádná
tichá změna chování). Celá dosavadní sada testů jádra (16 souborů)
prochází beze změny - do samotného procesoru, paměti ani POKEY
generátoru jsem vůbec nesahal.

## Co NEJDE ověřit odsud a potřebuju tvůj telefon

Samotné OpenSL ES volání (skutečný zvukový výstup na Androidu)
nejde na tomhle počítači ani zkompilovat - nemám tu Android NDK.
Tahle část prošla jen pečlivou opakovanou ruční kontrolou kódu, ne
skutečným spuštěním. Potřebuju od tebe vědět, až to vyzkoušíš pod
HELP:

- hraje zvuk vůbec?
- nehraje náhodou dvakrát (ozvěna/echo)?
- je hlasitost stejná jako dřív (jako v B286)?
- CSAVE→WAV nahrávka je pořád v pořádku?

Tohle je jediný build od B282, který doopravdy nejde ověřit ničím
jiným než tvým telefonem - CI mi potvrdí jen to, že se to správně
zkompiluje, ne jak to zní.
