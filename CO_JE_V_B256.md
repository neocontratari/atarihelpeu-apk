# B256 — Oprava mé regrese: boot nefungoval vůbec (versionCode 302)

Rene: "Teď nenaběhlo vůbec nic. Mě už vadilo, že podle tebe je to
dostačující - NE - to musí být výborný!! Partáku, začínáš dělat
chyby."

Má chyba, plně přiznaná - a beru ji vážně.

===============================================================================
 CO SE STALO
===============================================================================

Při přepisu `bootNative()` v B255 (smaž a postav znovu celý stroj,
místo ručního vyjmenovávání polí k vyčištění) jsem **omylem vynechal
řádek `g_stroj->reset()`**.

Bez tohohle volání procesor nikdy nenačte reset vektor z ROM
($FFFC/$FFFD) a zůstane na PC=0 (výchozí stav). Appka pak 600 snímků
běžela úplně naprázdno - přesně to bylo v tvém logu: `PC=0 DMACTL=0
DLIST=0`, při obou pokusech o boot, žádný zvuk, nic.

Tohle byla **regrese, ne oprava** - a prošla mnou, protože test, který
jsem měl v B255, ověřoval jen `Machine`/zvuk **izolovaně**, nikdy
nezkusil celou cestu bootu od začátku do konce se skutečnými ROM daty.
"Kompiluje se" není totéž co "funguje" - a tohle je přesně ten
případ.

===============================================================================
 OPRAVA
===============================================================================

Řádek `g_stroj->reset()` vrácen zpět, na správné místo (po vyčištění
paměti, před prvním spuštěním snímku).

===============================================================================
 NOVÝ TEST - PŘESNĚ TEN, CO MĚL EXISTOVAT UŽ PŘEDTÍM
===============================================================================

`test_atari_boot.cpp` (uložen trvale, ne jen pro tuhle opravu) -
skutečný test celé cesty bootu se **skutečnými ROM daty**, ne
atrapou:

1. Přečte skutečný reset vektor **přímo z ROM dat** (nezávisle na
   kódu jádra - takže test neověřuje sám sebe)
2. Ověří, že `reset()` dá procesoru přesně tuhle adresu
3. Spustí přesně stejnou posloupnost jako `bootNative()` (600 snímků)
   a ověří, že PC/DMACTL/DLIST nejsou nula a procesor se nezaseklo
4. Ověří, že **dva nezávislé "studené starty"** dají **identický**
   výsledek (přesně to, co minule chybělo a způsobilo "zelenou
   obrazovku")

Všechny čtyři kontroly prošly. DMACTL vyšlo na 34 - přesně stejná
hodnota, jakou appka dávala při dřívějších úspěšných bootech.

===============================================================================
 CO SI Z TOHO BEROU DO BUDOUCNA
===============================================================================

Kdykoli se mění funkce, kterou appka volá při startu (nebo cokoli
podobně zásadního), nestačí testovat vnitřní součásti izolovaně -
musí existovat test, který projde **celou tou cestou se skutečnými
daty**, přesně tak, jak to appka dělá doopravdy. "Dostatečné" nestačí
- musí to být výborné, přesně jak říkáš.
