# B257 — Čtyři věci z ostrého testu (versionCode 303)

Rene: "zvuk se seká, po CSAVE+RETURN nesmyslná smyčka, self test se
kouše, 2x zpět spadne appka, PS1 Doom rozsypaná grafika - nechci
rozbitou appku!!"

===============================================================================
 1) CSAVE "nesmyslná smyčka" - PŘÍČINA NALEZENA A ČÁSTEČNĚ OPRAVENA
===============================================================================

V logu byly registry AUDF/AUDC/AUDCTL zamrzlé beze změny přes 16+
vteřin po CSAVE+RETURN. Přidal jsem do logu i stav **JAM** (zaseknutí
procesoru) - a ukázalo se přesně tohle: procesor se zasekl (nejspíš
narazil na kód, který se snaží mluvit s kazetovým portem, jenž appka
zatím neemuluje).

Skutečná chyba ale byla jinde: appka **BEZ OHLEDU na zaseknutí
procesoru** dál generovala zvuk ze zamrzlých registrů - donekonečna
stejný tón. Skutečný hardware, když spadne, přestane hrát. Teď appka
dělá totéž - při zaseknutí procesoru je výstup ticho, ne nesmyslné
bzučení.

**Základní příčina (proč se procesor při CSAVE zasekává) zatím
vyřešená není** - to je emulace kazetového portu, což je větší,
samostatná práce. Tohle je oprava toho, jak se appka chová, KDYŽ k
zaseknutí dojde - ne oprava toho, PROČ k němu dochází.

Ověřeno testem se skutečnou ROM: simulace zaseknutí → výstup přesně
0.0 (ticho).

===============================================================================
 2) PÁD APLIKACE PO 2X ZPĚT
===============================================================================

Nemám stack trace (nemáš ADB, jen tenhle prohlížečový log), takže
nemůžu ukázat přesný řádek, kde to padalo. Přidal jsem dvě nezávislá
opatření místo hádání:

- **Ochrana proti rychlému dvojitému stisku** (stejný vzor, jaký už
  appka má u RESET tlačítka) - druhý stisk do 400ms se ignoruje.
- **Bezpečnostní síť** - celé zpracování je teď v try/catch, takže i
  kdyby něco neočekávaného selhalo, appka se v nejhorším případě jen
  neuposlechne ten jeden stisk, ne že spadne úplně.

===============================================================================
 3) SELF-TEST "SE KOUŠE"
===============================================================================

Přidána diagnostika **podtékání zvukového bufferu** - když appka
nestíhá generovat zvuk dost rychle na plynulé přehrávání, vznikne
mezera (to je přesně to "kousání"). Dřív se tohle tiše opravovalo bez
záznamu - teď se počet a velikost mezer hlásí do logu každých ~5
vteřin. Potřebuju tvůj další log, abych přesně poznal příčinu, místo
dalšího hádání.

===============================================================================
 4) PS1 DOOM - ROZSYPANÁ GRAFIKA
===============================================================================

Ověřil jsem důkladně: PS1 kódu (`nap_ps1_native.cpp`, `core_ps1.c`,
cokoli v `pcsx_rearmed`) jsem se během **celé** Atari práce ani
jednou nedotkl.

Našel jsem v PS1 kódu komentář (s mnohem staršími značkami sestavení
než moje, z doby dávno před touhle prací) - doslova: *"naše
'interlaced' větev, kterou nepoužíváme."* To silně ukazuje na
**předexistující, vědomě obejitou mezeru** v podpoře prokládaného
zobrazení - Doom ho pravděpodobně používá, jiné hry, co jsi testoval
dřív, možná ne. Nesouvisí to s Atari prací.

Nevyřešeno - čeká na tvé rozhodnutí, jestli to chceš řešit teď, nebo
až po Atari.

===============================================================================
 OVĚŘENÍ
===============================================================================

Nový C++ test (skutečná ROM, simulace zaseknutí) - ticho při JAM
potvrzeno. `nap_atari_native.cpp` znovu zkompilován čistě. Rovnováha
závorek v `MainActivity.java` ověřena chytrým kontrolorem - 0. TV
cesta - 0 chyb. JS syntaxe testovací stránky - čistá, žádná
duplicitní id.
