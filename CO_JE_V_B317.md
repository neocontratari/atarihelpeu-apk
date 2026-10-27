# Co je v B317

Tvoje zpráva: „Teď tu myšku pro PS1 Dune 2000 :-)“ – myš PlayStation ovládaná přes obrazovku.

## 1. Co umí

- **Myš PlayStation (SCPH-1090)** pro PS1 – celé v C++, jádro PS1 vidí skutečnou myš (ID 12),
  stejně jako konzole se zasunutou myší. Dune 2000 myš oficiálně podporuje.
- **Tlačítko MYŠ** – na šířku vlevo nahoře (naproti kolečku). Zapnutá myš = tlačítko svítí zlatě.
  - Poprvé se rovnou otevře nápověda s gesty.
  - Podruhé už jen krátká hláška.
- **Obraz hry je touchpad** (jako na notebooku):
  - **táhni prstem po obrazu** = pohyb kurzoru (tah přes celou šířku obrazu = 320 bodů myši);
  - **ťukni** = levé tlačítko (vybrat, potvrdit);
  - **ťukni dvěma prsty** = pravé tlačítko (zrušit);
  - **podrž prst na místě** – po 0,5 s zavibruje a nahoře se ukáže **DRŽÍM LEVÉ TLAČÍTKO**. Pak táhni =
    výběr jednotek rámečkem. Zvednutím prstu se tlačítko pustí.
  - Klik krátce zavibruje (vypíná se v OVLÁDÁNÍ: CITLIVOST A VZHLED → Vibrace při stisku tlačítka).
- **Myš v portu 1** (výchozí – port 2 prázdný, jak to radí emulátor ePSXe pro Dune 2000):
  - **✕** = levé a **○** = pravé tlačítko myši (dají se držet, i spolu s tahem prstu);
  - **D-pad** = pomalý přesný posun kurzoru, držením se zrychlí;
  - ostatní tlačítka (△ □ L1 L2 R1 R2 SELECT START) hra s myší nevidí, proto jsou schovaná;
  - totéž funguje na herním ovladači i na klávesnici TV / PC: šipky = kurzor, K nebo mezerník = levé,
    L = pravé.
- **Kolečko ⚙ → MYŠ PLAYSTATION (DUNE 2000 …)** – nastavení:
  - **kam je myš zapojená** – hry ji hledají různě:
    - MYŠ V PORTU 1 (DUNE 2000),
    - MYŠ PORT 1 + OVLADAČ PORT 2,
    - OVLADAČ PORT 1 + MYŠ PORT 2,
    - VYPNUTÁ – JEN OVLADAČ;
  - **rychlost kurzoru** 25–400 %;
  - která hra běží a co je zapojené.
- **Pamatuje se pro každou hru zvlášť** (podle ID disku, např. SLES02250 = Dune 2000):
  - příště je myš zapojená hned od zapnutí hry;
  - ostatní hry mají dál normální ovladač.
- **Vypnutá myš = PS1 přesně jako v B316** (ovladač, tlačítka, klávesnice TV).

## 2. Poctivě

- **Dune 2000 jsem tady vyzkoušet nemohl** – hru nemám (a do repozitáře ji nedávám). Na PC jsem myš
  zkoušel se skutečným jádrem PS1 z appky a malým programem, který myš čte přes BIOS jako hry.
- **Ve kterém portu Dune 2000 myš hledá, nevím jistě.**
  - Výchozí je port 1 s prázdným portem 2.
  - Když hra kurzor nevidí, zkus v nastavení další volbu.
  - Do logu se píše, jak často hra čte port 1 a port 2 (řádek „MYS ZDRAVI … hra cte port 1 Nx/s,
    port 2 Nx/s“) – z toho poznám, kde myš hledá.
- **Zapnutí myši za běhu je jako vytažení ovladače a zasunutí myši.** Asi půl sekundy hra nevidí nic.
  - Některé hry pak ukážou „ovladač odpojen“.
  - Když hra myš nevezme, spusť ji znovu (CD ISO) – myš už je pak zapojená od zapnutí.
- **Myš počítače na TV / PC** (pohyb myší v prohlížeči) zatím není. Jádro na ni je připravené, chybí
  jen stránka TV. Klávesnice TV myš ovládat umí (šipky, K / mezerník, L).
- **Oprava v jádře:**
  - když hra přečte myš dvakrát v jednom snímku, dostane posun jen jednou, jako u skutečné myši;
  - dřív by kurzor jel dvakrát rychleji;
  - ovladače se to netýká.

## Co prosím vyzkoušet

1. Nainstaluj B317 (postup zůstává).
2. PS1 → spusť **Dune 2000** → otoč telefon na šířku.
3. Klepni na **MYŠ** (vlevo nahoře) – otevře se nápověda → ZAVŘÍT.
4. Táhni prstem po obrazu – hýbe se kurzor? Ťukni = vybrat, ťukni dvěma prsty = zrušit.
5. Podrž prst, až zavibruje, a táhni přes jednotky – vybere je rámečkem?
6. Zkus **✕ / ○** a **D-pad** (přesný posun kurzoru).
7. Když se kurzor nehýbe: kolečko → **MYŠ PLAYSTATION** → zkus **MYŠ PORT 1 + OVLADAČ PORT 2** nebo
   **OVLADAČ PORT 1 + MYŠ PORT 2**. Rychlost kurzoru si nastav podle sebe.
8. Ukonči hru a spusť ji znovu – myš má být zapnutá sama.
9. Kolečko → **ULOŽIT LOG A ODESLAT** → pošli log (řádky „MYS …“).

## Ověřeno na PC

- **Logika myši v C++ – 82/0:**
  - tah, ťuknutí, dvojklik, dva prsty, podržení a tah, zrušení dotyku;
  - porty všech 4 režimů;
  - ✕ / ○ / D-pad;
  - myš z PC;
  - paměť pro hru (i poškozený soubor, 500 her);
  - souhrn do logu;
  - ThreadSanitizer: 0 hlášení.
- **Skutečné jádro PS1 z appky (PCSX-ReARMed, BIOS HLE) – 30/0.** „Hra“ čte ovladače přes BIOS jako
  skutečné hry a sčítá posun myši:
  - s vypnutou myší ovladač jako B316;
  - po zapnutí hra vidí myš (ID 12);
  - tah 10 % šířky obrazu = přesně +32 bodů;
  - rychlý tah = −640 po nejvýš 127 za snímek;
  - ťuknutí = 1 klik (5 snímků), dva prsty = pravé;
  - podržení + tah = výběr rámečkem;
  - ✕ / ○, D-pad, myš z PC;
  - čtení 2× v jednom snímku = posun jen jednou (B316 by dal 2×);
  - všechny 3 porty myši;
  - znovu spuštěná hra má myš od prvního snímku;
  - telefonní cesta (vlákno emulace, dotyky z jiného vlákna);
  - ThreadSanitizer: 0 hlášení.
- **Stránka PS1 v Chromiu se skutečnými dotyky (i dva prsty naráz) – 31/0:**
  - každé gesto přehrané do skutečné logiky myši dá to, co má;
  - tlačítko MYŠ, schovaná tlačítka, ukazatel držení + vibrace, panel;
  - česky beze změny proti B316 (jen texty myši navíc);
  - anglicky bez češtiny, žádná chyba JavaScriptu.
- **Regrese:**
  - testy PS1 z B308 na kódu B317 – deník, klávesnice z TV, zámek, ThreadSanitizer, JVM, klávesnice
    v prohlížeči, stránka PS1 – bez chyby;
  - testy TV B305 / B306 jdou přeložit;
  - Java proti skutečnému Androidu: 0 chyb;
  - všech 27 propojení Java ↔ C++ u PS1 sedí.
- **Návod:**
  - v kapitole PS1 přibyl jen blok o myši (česky i anglicky);
  - názvy tlačítek v návodu v appce opravdu jsou.
