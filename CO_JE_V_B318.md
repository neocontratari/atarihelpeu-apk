# Co je v B318

Tvoje zpráva: „Myš sice funguje, ale je dost neomalená, i při nastavení citlivosti na 300 je to divný …
Přenos LAN buď nefunguje, a nebo nevím jak na to … a zase vidím JAVA – tok“.

## 1. Myš PS1 – plynulejší a přesnější

- **Proč to bylo divné (z tvého logu):**
  - Dune 2000 kreslí přesně **20 obrázků za sekundu** – nový obraz jen každý 3. snímek.
  - Myš si hra nejspíš čte taky jen jednou za svůj obrázek. Pohyb z ostatních dvou snímků se ztratí
    (stejně jako na skutečném PlayStationu).
  - Kolik z pohybu prstu hra uviděla, záleželo na tom, do kterých snímků dotyky zrovna padly.
    Kurzor se proto cukal – jednou skočil, jindy stál. Proto jsi potřeboval 300 % a i tak to bylo divné.
  - Telefon za to nemůže.
- **Teď appka pozná, jak často hra kreslí**, a posun myši rozloží rovnoměrně do těch snímků:
  - hra vidí každý pohyb prstu stejně – kurzor jede plynule a přesně;
  - na PC se skutečným jádrem PS1: B317 – kurzor ujížděl od prstu až o 5 bodů, B318 – nejvýš 0,7 bodu;
    v logice myši s chvěním dotyků jako v telefonu B317 až 36 bodů, B318 nejvýš 2,3;
  - průměrná rychlost se nemění – tvých **295 % platí dál**.
- **Žádný skok na začátku tahu.** B317 stál, dokud prst neujel 3 % obrazu, a pak kurzor naráz
  poskočil (při 295 % o 28 bodů). Teď kurzor stojí jen první 1,5 % a pak se rozjede plynule.
  Ťuknutí tak zůstane přesné.
- **Zrychlení jako u myši počítače:**
  - pomalý tah = jemný přesný posun (vybrat jednu jednotku);
  - běžný tah = přesně tvoje rychlost;
  - rychlý tah = kurzor dojede dál (přes mapu jedním tahem);
  - vypíná se v nastavení myši tlačítkem **ZRYCHLENÍ: ZAPNUTO** – pamatuje se pro každou hru.
- **Ťuknutí klikne až po dojetí kurzoru** – klikne tam, kam prst ukázal.
- **Podržení (výběr rámečkem) až po 0,65 s** a jen s prstem na místě (B317: 0,5 s). Méně omylem
  „držím levé“, když chvilku přemýšlíš s prstem na obrazu.
- **Log:** každý tah (`MYS TAH …` – jak dlouho, jak rychle, zrychlení, kolik bodů dostala hra)
  a `hra kresli kazdy 3. snimek`. Z toho poznám, co doladit.

## 2. Přenos LAN – myš počítače

- **Přenos fungoval** – v logu šel obraz PS1 na PC rovnou z C++ (59 paketů za sekundu) i se zvukem.
- **Myš počítače ale do hry nešla** – to ještě nebylo hotové (psal jsem v B317). **Teď je:**
  1. Zapni WEB TV, na PC otevři adresu.
  2. Na telefonu PS1 → Dune 2000 → myš zapnutá (tlačítko MYŠ).
  3. V prohlížeči na PC nahoře uvidíš **MYŠ PS1: KLIKNI DO OBRAZU …** → klikni do obrazu.
  4. Myš počítače teď ovládá kurzor hry: levé tlačítko = levé, pravé = pravé (i držení, výběr rámečkem).
  5. **Esc** myš pustí. Znovu klik do obrazu = zase hraješ.
- Rychlost je stejná jako prstem (tvé nastavení pro hru).
- Klávesnice PC funguje dál (šipky, K / mezerník, L).

## 3. „JAVA“ v logu

- V řádku menu bylo **„TV Java 1280x720“**. Znamenalo to: Java brala obraz menu pro TV – i když se
  nikdo nedíval. V tvém logu 14 s mezi zapnutím WEB TV a připojením prohlížeče, a menu přitom
  nestíhalo (24 z 60 snímků).
- **Teď Java pro TV nesnímá nic, když se nikdo nedívá**, ani když prohlížeč bere obraz rovnou z C++.
  Snímá jen pro starý prohlížeč, který proud z C++ neumí.
- Stránka v prohlížeči jde rovnou na proud z C++ (dřív si na první vteřinu otevřela starou cestu přes
  Javu – odtud to „Java“).
- Řádek menu teď píše srozumitelně: `TV: proud z C++ …` / `TV: nikdo se nediva …` /
  `TV: stara cesta pres Javu (prohlizec bez proudu)`.
- Java zůstává jen tam, kde musí (okno Androidu, síť). Obraz a zvuk her jdou z C++.

## 4. Poctivě

- **Dune 2000 nemám**, takže nemůžu ověřit, že hra opravdu čte myš jen každý 3. snímek. Odpovídá tomu
  tvých 295 % i to cukání. Kdyby četla každý snímek, rozložení nic nezkazí – kurzor jen o snímek později.
- **Zrychlení:** střed jsem nastavil na běžný tah (0,7 šířky obrazu za sekundu). Když je to moc nebo
  málo, pošli log (řádky `MYS TAH`) – doladím. Se zrychlením možná sneseš menší rychlost než 295 %.
- **Zachycení myši v prohlížeči** (zámek ukazatele) umí Chrome, Edge i Firefox na PC. Prohlížeč přímo
  v televizi to umět nemusí – tam klik do obrazu přepíná celou obrazovku jako dřív.
- Na telefonu (dotykový displej, vibrace) jsem to zkusit nemohl.

## Co prosím vyzkoušet

1. Nainstaluj B318 (postup zůstává).
2. Dune 2000 → MYŠ: krátké jemné posuny, dlouhé rychlé tahy přes mapu, ťuknutí na jednotku,
   výběr rámečkem (podrž prst, až zavibruje).
3. Kolečko → **MYŠ PLAYSTATION** → zkus **ZRYCHLENÍ** zapnout i vypnout. Rychlost si dolaď.
4. WEB TV na PC: klikni do obrazu, hraj myší počítače, **Esc** myš pustí.
5. Kolečko → **ULOŽIT LOG A ODESLAT** → pošli log.

## Ověřeno na PC

- **Logika myši v C++ – 83/0:**
  - žádný skok (prst po 1 bodu = kurzor nejvýš 1 bod za snímek);
  - zrychlení (pomalu 0,4×, normálně 1×, rychle 2×; vypnuté = vždy stejně);
  - rozložení podle hry (2 / 3 / 4 snímky, nepřepíná sem a tam), součet přesný;
  - hra, která čte jen každý 3. snímek, s chvěním dotyků jako v telefonu: B318 přesně na třetině
    pohybu (odchylka nejvýš 2,3 bodu), B317 ujížděl až o 36 bodů;
  - ťuknutí (i s malým posunem – klik po dojetí), dva prsty, podržení 0,65 s, ✕ / ○ / D-pad, myš z PC;
  - paměť pro hru (i soubor z B317 – tvé Dune zůstane 295 %);
  - ThreadSanitizer: 0 hlášení.
- **Skutečné jádro PS1 z appky – 36/0** (i pod ThreadSanitizerem, 0 hlášení). „Hra“ čte myš přes BIOS
  jako skutečné hry, a nově umí kreslit jen každý 3. snímek jako Dune 2000:
  - jádro hlásí nový obraz, myš pozná „hra kreslí každý 3. snímek“;
  - tah s nerovnoměrnými dotyky: hra přesně na třetině pohybu (odchylka 0,7 bodu), B317 až 4,7;
  - klik vidí i hra, která čte jen každý 3. snímek;
  - tah, rychlý tah, ťuknutí, dva prsty, podržení, ✕ / ○ / D-pad, myš z PC, všechny porty, vypnutí,
    znovu spuštěná hra si pamatuje i zrychlení, telefonní cesta s vláknem emulace.
- **Stránka PS1 v Chromiu se skutečnými dotyky – 29/0:**
  - gesta přehraná do skutečné logiky myši B318;
  - tlačítko ZRYCHLENÍ (stav z C++, hlášky), volba portu se neplete;
  - česky = B317 + jen nové texty, anglicky bez češtiny, žádná chyba JavaScriptu.
- **Stránka WEB TV v Chromiu (přesně z appky) – 34/0:**
  - s proudem z C++ stránka starou cestu přes Javu vůbec neotevře (B317 hned po načtení);
  - klik do obrazu zachytí myš, pohyb jde do hry přesně, tlačítka, držení, Esc pustí;
  - myš na telefonu vypnutá = myš PC se sama pustí, bez myši PS1 klik přepíná celou obrazovku jako dřív;
  - anglicky přeloženo, žádná chyba JavaScriptu.
- **Java:** skutečná metoda „stará cesta jen pro diváka“ z appky vyzkoušená v JVM (19/0);
  Java proti skutečnému Androidu bez chyby; všech 28 propojení Java ↔ C++ u PS1 sedí.
- **Skutečné hlavní menu na PC:** vše jako dřív, řádek `TV: nikdo se nediva …` / `TV: stara cesta pres Javu`.
- **Regrese – 19 OK, 0 CHYBA:**
  - PS1 s vypnutou myší jako dřív (deník a klávesnice PC z B308 63/0, ThreadSanitizer 0, stránka PS1 23/0);
  - Java ↔ C++ sedí (28 propojení, celá appka přeložená proti Androidu bez chyby);
  - přenos na TV / PC jako dřív: zvuk v dalších hrách 20/0, přepnutí proudu po menu 3/0,
    klávesnice PC do Atari 23/0, Atari na TV 23/0, Sega / PS1 na TV 37/0, klávesnice PC do PS1 17/0;
  - stránka WEB TV česky i anglicky jako dřív, skutečné hlavní menu 15/0.
- **Návod:** v kapitole PS1 a WEB TV jen to o myši (česky i anglicky); názvy tlačítek v návodu v appce
  opravdu jsou.
