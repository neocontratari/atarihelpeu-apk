# Co je v B299

Tvoje zpráva po B298: „Pokud nahraji program v obyčejném BASICu, tak LIST
projde. Ale pokud ho správně nahraji v Turbo BASICu, tak začne v listu po
chvilce házet artefakty. Kód uzamčený není, je to můj program. … Při vkládání
TXT programu potřebuju, aby si šel vybrat TXT soubor s kódem pro Turbo BASIC
i pro BASIC, aplikace ho automaticky přepsala do obrazovky, šel spustit,
uložit a nahrát – pod tlačítkem BASIC/TBXL TXT. … Načítání z mých stránek
pořád trochu zlobí – možná Wi-Fi.“

## 1. BASIC/TBXL TXT – program z TXT souboru

Tlačítko **BASIC/TBXL TXT** teď nabídne:

- **TXT SOUBOR → ATARI BASIC**: vybereš TXT v telefonu. Atari naběhne
  s BASICem a appka program sama napíše klávesnicí Atari (NEW, pak řádek po
  řádku, je to vidět na obrazovce). Pod přístrojem pak je
  „HOTOVO … ŘÁDKŮ – RUN / CSAVE“.
- **TXT SOUBOR → TURBO-BASIC XL**: nejdřív se nahraje Turbo-BASIC XL, potom
  se stejně napíše program.
- **NAPSAT / VLOŽIT TEXT RUČNĚ**: původní okno pro vložení textu.

Potom už normálně: RUN, CSAVE (uloží WAV), CLOAD.

Jak to píše:

- Jako člověk u klávesnice, jen rychle: tvůj WALLS (71 řádků, 3 539 kláves)
  je v Turbo-BASICu napsaný za 20 s. LIST pak souhlasí se souborem znak po
  znaku (ověřeno na PC).
- Píše jen řádky s číslem. Prázdné řádky, nadpisy a „READY“ v souboru
  přeskočí a řekne to v logu.
- Příkazy a proměnné píše velkými písmeny. Malá písmena v řetězcích
  („Ahoj“), za REM a v DATA nechá tak, jak jsou v souboru.
- Řádek může mít až 120 znaků (během psaní je POKE 82,0, pak se vrátí).
  Delší řádek na Atari napsat nejde: přeskočí ho a v logu je jeho číslo.
- Zvládne i znaky Atari (ATASCII): inverzní znaky, grafiku přes CTRL a
  řídicí znaky v řetězcích přes ESC, např. `}` jako smazání obrazovky
  v PRINT. Pozná i soubor uložený přímo z Atari (konce řádků $9B).
- Na konci projde program v paměti. Řádky s chybou syntaxe (BASIC je uloží
  jako „ERROR-“) vypíše v logu číslem, pod přístrojem je „CHYBA V n ŘÁDCÍCH“.
- Uložený program (.BAS z `SAVE`) není text. Appka to pozná a nic nepíše.

## 2. Rozsypaný LIST v Turbo-BASICu (tvůj WALLS)

Co přesně je na tvém snímku: Turbo-BASIC tam pořád dokola vypisuje surové
bajty řádků 1000–1080 programu (BODY, REKORD, CAS, ATARI, WALLS, 130XE…),
vždy přesně 256 bajtů. To dělá jeho LIST, když má vypsat jméno proměnné
č. 128. Tvůj program jich má 27, takže hledá „jméno“ za koncem tabulky
proměnných a projde až do programu. Taková proměnná se objeví, jen když
LIST přečte dvě nuly jako příkaz. Jinými slovy: LIST se v nějakém řádku
„rozjel“ (četl uprostřed čísla), nebo četl paměť, kde tvůj program není.

Co jsem udělal:

- **Tvůj WAV**: mám ho rozebraný bajt po bajtu. 36 záznamů, součty
  v pořádku, program 4 417 B. Na PC postupem jako ty (TURBO BASIC, CLOAD,
  vložení kazety až po pípnutí, RETURN, STOP, LIST): výpis je pokaždé celý
  a správný, 71 řádků. Zkoušel jsem:
  - skutečné vlákna appky,
  - jádro přeložené pro ARM jako ve tvém telefonu,
  - 300× LIST v náhodném okamžiku a s různě dlouho drženými klávesami,
  - 24× celý postup s náhodným časováním,
  - RUN + BREAK / RESET a pak LIST.
  Chybu jsem u sebe nevyvolal.
- **Našel a opravil jsem skutečnou chybu přesně v tomhle místě.** Turbo-BASIC
  běží s vypnutou ROM a v RAM pod ní má na adrese $E459 vlastní kód. Je to
  rutina, kterou LIST, IF i RUN čtou čísla a proměnné z programu. Na
  stejné adrese má OS v ROM vstup do SIO a emulátor tam má zkratku pro
  disketu. Ta Turbo-BASIC „přepadla“, když byla v D1: disketa. Výsledek
  byl rozsypaný LIST i RUN. Test na PC: Turbo-BASIC + disketa v D1:
  v B298 se rozsype hned na řádku s IF, v B299 je LIST i RUN správně.
  Zkratka teď platí jen se zapnutou ROM OS. Pomůže to i hrám z ATR, které
  vypínají ROM.
- **Upřímně:** podle tvého logu jsi v tu chvíli disketu v D1: neměl.
  Nemůžu tedy tvrdit, že tohle byla tvoje chyba. Proto je v B299
  diagnostika:
  - **každý řádek, který napíšeš na klávesnici přístroje**, je v logu
    (`B299 KLAVESY: CLOAD <RETURN>`, `B299 KLAVESY: LIST <RETURN>`),
  - **LOG/CHYBA přiloží k logu celou paměť Atari** (64 kB + rozšířených
    64 kB, zabalené) a souhrn: stav procesoru, ukazatele BASICu, kontrolní
    součty. Ty čísla mám z PC pro tvůj WALLS po CLOAD, takže uvidím,
    jestli se v telefonu liší program, kód Turbo-BASICu, nebo nic.

**Prosím:** když se LIST zase rozsype, **nemačkej RESET**. Hned dej
LOG/CHYBA a log mi pošli. Napiš mi, **který řádek byl poslední správný**.

## 3. Hry z tvých stránek přes Wi-Fi

V logu je to černé na bílém. Na tvé Wi-Fi telefon u atarihelp.eu vůbec
nedostal tvůj web na WEDOS. Odpověděl mu jiný server s certifikátem
**„localhost“ od „ospanel“**. To je Open Server Panel, místní webový server
na PC (asi tam máš kopii atarihelp.eu na vývoj). Telefon se tedy na tvé
Wi-Fi dostane k tomu PC místo na internet. Appka takové spojení správně
odmítne. Stránky se pak načítaly jen oklikou přes allorigins, a to jen
někdy. Proto „někdy jde a někdy ne“.

- Zkus to na **mobilních datech**. Tam to jít má.
- Na Wi-Fi zkontroluj, kam vede atarihelp.eu:
  - v OSPanelu vypni doménu atarihelp.eu,
  - nebo DNS v routeru,
  - nebo jestli telefon nejde přes hotspot z PC, kde OSPanel běží.
- Appka to teď rovnou řekne: „na této Wi-Fi vede atarihelp.eu na MÍSTNÍ
  server…“. Pod přístrojem je „WI-FI: ATARIHELP.EU = MÍSTNÍ SERVER“.
  Dřív v HELP chyba stahování nebyla vidět vůbec.
- corsproxy.io už bez klíče vrací jen „401“, proto jsem ho vyřadil.
- Odkaz `ahgame://` ze stránky (v logu „unknown protocol: ahgame“) se teď
  rozbalí na skutečnou adresu hry.

## Ověřeno na počítači

- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: **61 kontrol,
  0 chyb**, z toho 5 nových:
  - TXT → ATARI BASIC (LIST + RUN),
  - TXT → Turbo-BASIC XL (DPOKE/DPEEK, TEXT),
  - klávesy v logu,
  - paměť pro LOG/CHYBA.
- TXT do BASICu i TBXL se zlobivým souborem: malá písmena, CRLF, řádek bez
  čísla, dlouhý řádek, chyba syntaxe, `}` v řetězci. Všechno se napíše
  nebo nahlásí, jak má.
- Tvůj WALLS jako TXT → Turbo-BASIC: LIST souhlasí znak po znaku.
- Turbo-BASIC + disketa v D1: B298 rozsypaný, B299 správně.
- Certifikát „localhost ← ospanel“ appka odmítne a hlásí ho jako místní
  server (test 7/7).
- Acid800 stejně jako dřív (42).
