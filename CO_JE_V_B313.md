# Co je v B313

Tvoje zpráva k B312: „I před zadáním kódu je aplikace odemčená … apku vždy mažu a nepřeinstalovávám.
Grafika hlavního menu je rozmazaná na TV či PC přenosu LAN. 5 – Ano, udělej to prosím – víc detailů
na těle, kardridž s nálepkou, otevírací víko na CD, kontrolky, lepší materiály a světlo, animace
(zasunutí kazety, roztočení CD) v menu i v introdukci. … pokud potřebuješ přesnější log, nasměruj to
do 8765/log. Jo a Parťáku, šlo by dát výběr jazyka – CZE a ANG?“

## 1. Odemčeno ještě před zadáním kódu – opraveno

- **Příčina:** záloha Androidu (Auto Backup). Appka zálohu nezakazovala, takže Android po smazání
  a nové instalaci vrátil uložený postup ze zálohy v tvém Google účtu – i odemčenou Segu a PS1.
  V tvém logu je to vidět i jinde: hned po instalaci appka „pamatovala“ cestu k Sonicovi a BIOSu.
- **Oprava:** záloha je vypnutá (`allowBackup="false"`). Při každém startu je v logu řádek
  `B313 POSTUP_PRI_STARTU … | nová instalace / aktualizace`, takže je vidět, odkud postup je.
- Když B313 nainstaluješ **po smazání** staré appky, začne zamčená. Když ji nainstaluješ **přes**
  starou (aktualizace), postup zůstane – zamknout se dá jednou: SERVIS → ZAMKNOUT VŠE.

## 2. Hlavní menu na TV / PC je ostré

- **Dřív:** obraz menu pro TV měl 854×480 bodů (na TV se zvětšoval 2,25×), JPEG v kvalitě 60–76
  a nikdy se nedoostřil – v menu se pořád něco hýbe (mlhovina, hvězdy, lesky, otáčející se konzole),
  takže appka nikdy neposlala „klidný“ snímek v plné kvalitě. V tvém logu: „zjemnění 0“.
- **Teď:**
  - velikost obrazu podle **KVALITA OBRAZU (WEB TV)** v OPTIONS: LOW 854×480, MED 1024×576,
    **HIGH 1280×720** (ty máš HIGH); přepne se hned, i když je menu na TV zrovna puštěné,
  - když nelistuješ, na TV se zastaví ozdoby (mlhovina, hvězdy, lesky) – konzole na kartě se točí dál –
    a všechno, co stojí, se do 1 s doostří na kvalitu 94 s plnými barvami (zlatá písmena bez šmouhy),
  - posílají se jen části, které se změnily (několik obdélníků – konzole na kartě, vedlejší karta),
    ne celý obraz: v klidu asi 20 kB na snímek,
  - při listování nižší kvalita (75) – pohyb oko stejně nepřečte a jede to plynuleji; jakmile se
    zastaví, doostří se,
  - na telefonu se ozdoby hýbou dál, zastavují se jen v obrazu pro TV.
- Ověřeno ve skutečném Chromiu se skutečným menu: obraz v prohlížeči proti originálu – nápisy nahoře
  43,7 dB, dole 47,8 dB (kvalita 94; dřív kolem 30 dB).

## 3. TV / PC: z menu na Atari bez čekání

- V tvém logu B312: po odchodu z menu do Atari 130XE prohlížeč ještě 25 s zkoušel menu a Atari šlo na
  TV starou pomalou cestou. Teď appka prohlížeči na proud, který není na obrazovce, hned odpoví, co na ní
  je, a prohlížeč se přepne (ověřeno: do 0,5 s, i když by se dotaz na stav zadrhl).

## 4. Menu na telefonu – rychlost

- Tvoje S8 si myslela, že menu nestíhá („úroveň 0“ pořád), protože si appka sama odhadovala, jak rychle
  displej překresluje, a odhad ujel na 6–8 ms. Proto kreslila živé karty méně často i na telefonu.
  Teď bere skutečnou obnovovací frekvenci z Androidu (60 Hz) – úroveň půjde na 3 (všechno živě).

## 5. Konzole N&P 16 a N&P 32 – nové, s detaily a animacemi

Pořád vlastní návrh N&P (žádná kopie cizích výrobků, žádná cizí loga, vymyšlená hra STAR RUNNER).

**N&P 16 (cartridge)**
- dvoupatrová skříň v půlnočně modré barvě s metalízou, zlatý pásek kolem dokola, kartáčovaná hliníková
  deska se zlatým emblémem N&P a potiskem 16-BIT, šroubky,
- vzadu „věž“ se štěrbinou: zlatý rámeček a dvě gumové klapky, které se při zasunutí otevřou dovnitř,
- **cartridge s nálepkou** (STAR RUNNER – západ slunce nad mřížkou, hory), štítek i na horní hraně
  a vzadu, zlaté kontakty, rýhy na úchop,
- vpředu potištěný panel: posuvný vypínač POWER, tlačítko RESET, **kontrolky ON (zelená) a CART
  (oranžová)**, dvě zdířky ovladačů se zlatými rámečky; větrací drážky, gumové nožičky.

**N&P 32 (CD)**
- grafitová skříň, lesklá černá deska, větrací mřížka, emblém N&P, nápis N&P 32,
- vpravo **víko na pantech s kulatým okénkem z kouřového skla**, chromový rámeček a azurový svítící
  prstenec; pod víkem mechanika (hřídel, laser na kolejnici),
- **disk s potiskem** N&P 32 SYSTEM DISC – když se roztočí, potisk se rozmaže jako u skutečného CD,
- vpředu černý štít: POWER se svítícím prstencem, OPEN, **řada 4 kontrolek**, kontrolka čtení disku,
  dvě zdířky ovladačů a dvě štěrbiny paměťových karet, azurový pás dole.

**Materiály a světlo:** studiové odlesky (velký měkký „softbox“ a světelný pruh – v lesklých plochách se
při otáčení pohybují), silnější odlesk při pohledu z boku jako u skutečných materiálů, zastínění
u podlahy (konzole „sedí“, neplave), metalíza, kartáčovaný kov, chrom a zlato, guma, kouřové sklo.

**Animace**
- **V menu:** když karta přijede doprostřed – N&P 16: cartridge sjede shora do štěrbiny, klapky se
  otevřou, cvakne, vypínač doprava, rozsvítí se ON a CART 3× blikne. N&P 32: disk sjede do otevřené
  mechaniky, víko se zavře, rozsvítí se POWER, kontrolky naskočí jedna po druhé a disk se roztočí.
  Když karta odjede, konzole je zase „vypnutá“ a při návratu se to přehraje znovu.
- **V intru** (kolem 30. s): hned po zhmotnění z neonu se zasune cartridge (kamera je zrovna u N&P 16)
  a N&P 32 vloží disk a zavře víko; když k ní kamera dojede, rozsvítí se a disk se roztočí – a hned
  potom se zapne obrazovka CRT.
- Obrázky karet pro záložní menu a pro HELP jsou taky nové.

## 6. „Test ATARI 130XE“ – co to je

Je to seznam kroků na stránce emulátoru: ATARI 130XE → LOG / CHYBA → „CO TESTOVAT“. Psal jsem tam,
co po nové verzi vyzkoušet. Nic se tam nemusí – od teď ti kroky píšu přímo do zprávy.

## 7. Log z 8765/log

Ano – `http://<adresa telefonu>:8765/log` je celý log appky (ten samý soubor), nejpřesnější, co můžeš
poslat. Otevři ho na PC, ulož jako text a pošli.

## 8. Jazyk CZ / EN

Ano, jde to – bude v příští verzi B314 pro celou appku (menu, OPTIONS, HELP, emulátory, hlášky, TV
stránka): volba v OPTIONS, poprvé podle jazyka telefonu.

## Co prosím vyzkoušet

1. Starou appku smaž, nainstaluj B313 a otevři: SEGA i PS1 musí být zamčené.
2. OPTIONS → KVALITA OBRAZU (WEB TV) = HIGH, SPUSTIT WEB TV, otevři adresu na PC / TV. Menu nech chvíli
   stát – do 1 s se musí doostřit (zlaté nápisy, popis pod kartou). Pak listuj šipkami.
3. Z menu na TV otevři ATARI 130XE – TV se musí přepnout na Atari do 1 s.
4. Na telefonu přejeď na kartu SEGA MEGA DRIVE (cartridge, kontrolky) a PS1 (disk, víko, roztočení).
5. OPTIONS → PUSTIT INTRO – kolem 30. s konzole.
6. Pošli log z `8765/log`.

## Ověřeno na PC

- **Záloha Androidu vypnutá, log při startu:** 4/0.
- **TV přepnutí z menu** (Chromium, stránka přesně z appky): 3/0 – i když stav visí na „menu“ jako
  u tebe, přepne se za 0,5 s.
- **Kodek pro menu na TV** (skutečný kód appky + „prohlížeč“ v C++, který pakety rozebírá jako stránka):
  11/0 – víc obdélníků, doostření i když se konzole točí, kvalita 75 / 85 / 94, PS1 a demo beze změny,
  změna velikosti, pomalá síť, předání snímku bez kopie, vlákna; i pod ThreadSanitizerem 0 hlášení.
- **Skutečné menu přes JNI** (jako z Javy): 15/0 – velikost TV podle kvality i za chodu, starý prohlížeč,
  perioda displeje ze 60 Hz; ThreadSanitizer: 0 závodů v našem kódu.
- **Menu na TV v Chromiu** (skutečné menu, stránka z appky): 11/0 – 1280×720, ostrost v klidu
  43,7 / 47,8 dB, přepnutí kvality za chodu, klávesnice PC.
- **Konzole:** animace a kolize dílů 10/0, obrázky karet a celé intro (237 snímků) bez chyby GL,
  shadery přísně podle OpenGL ES 2.0: 0 chyb.
- **Java proti skutečnému Androidu:** 0 chyb, podpisy JNI 23/23.
- **Regrese:** PS1 / Sega / demo na TV (kodeky 35/0, rozbočovač 16/0 + TSan), intro z Javy 11/0,
  karusel, snímky menu, názvy tlačítek v návodu proti appce.

Postup: `test_overeni/b313/JAK_SPUSTIT.txt`, výstupy `test_overeni/b313/VYSTUP_*`.
