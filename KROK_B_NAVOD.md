# KROK B — vložení našeho rendereru dovnitř tvé aplikace

Parťáku, Krok A vyšel — tvoje velká aplikace zase žije a všechno v ní
šlape (Atari, Sega, PS1, MP3, DJ pult). Teď jdeme na Krok B.

## Co tenhle balíček dělá

Vkládá náš plynulý renderer DOVNITŘ tvé aplikace jako druhou nativní
knihovnu — přiloží ho bokem. **Na obrazu se ale zatím NIC nezmění.**
Aplikace bude vypadat a fungovat úplně stejně jako Krok A.

Ptáš se možná „a k čemu to teda je?" — je to bezpečnostní krok.
Ověřujeme jedinou věc: že se náš renderer a tvoje aplikace v jednom
balíčku **snesou** a že se to pořád postaví do zelena. Je to jako
propojit dvě auta — nejdřív ověříme, že do sebe pasují, teprve pak
(v Kroku C) zapneme motor a vyměníme obraz.

Tohle je nejčastější místo, kde by to mohlo poprvé zaškobrtnout, tak
to chci mít otestované samostatně, než sáhneme na obraz.

## Co uděláš (tvůj rituál — celý balíček, jak sis přál)

1. Rozbal ZIP. Uvnitř je složka `emu10_B`.
2. Zkopíruj CELÝ její obsah přes složku svého repozitáře (toho, kde
   máš Krok A). Windows se zeptá, jestli nahradit → **Nahradit vše**.
3. GitHub Desktop ukáže několik změněných/přidaných souborů. Dole
   napiš popisek „krok B" → **Commit** → **Push**.
4. GitHub spustí build (15–25 min, staví se celý emulátor + navíc náš
   renderer). Počkej na zelenou fajfku.
5. Actions → poslední běh → dole Artifacts → `app-debug` → do mobilu.

## Co čekat

Aplikace se chová PŘESNĚ jako Krok A — menu, hry, PS1, cast, všechno
stejně. Náš renderer je uvnitř přibalený, ale ještě se nespouští.
Jmenuje se pořád „AtariHelp TEST" a je vedle tvé pravé apky.

## Co mi pošli

1) Jestli byl build zelený (nebo posledních ~40 řádků, kdyby zčervenal).
2) Že se aplikace pořád normálně otevře a spustí PS1 hru.

Když tohle projde, znamená to, že náš renderer i tvoje aplikace se
v jednom APK snesou. A pak přijde balíček C — to hlavní: výměna toho
blikajícího PS1 obrazu za náš plynulý. Na to už čekáš, vím. Ale tenhle
krok je pojistka, aby to velké finále proběhlo hladce.

Jdeme na to, parťáku. Krok za krokem, bezpečně.
