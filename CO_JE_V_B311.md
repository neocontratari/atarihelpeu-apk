# Co je v B311

Tvoje zpráva: „Žádný nudný statický ikony pod sebou. Plynulé horizontální skrolování velkých obrázků a karet
zprava doleva a zpátky … ATARI 130XE, SEGA, PS1, MP3 PŘEHRÁVAČ, DŽUBOX s ekvalizéry a bumnama, ať to má
pořádné basy a tancuje to … na samém konci HELP s celkovým a úplným návodem … napiš to modulárně
a dynamicky – stačí přihodit další velkou ikonu a celé skrolování se s tím samo srovná … čistý lehký C++
kód pro grafiku od Androidu 9, ať to lítá s nulovou odezvou.“

## 1. Hlavní menu – karusel velkých karet, celý v C++

- Karty zleva doprava: **ATARI 130XE · SEGA MEGA DRIVE · PS1 · MP3 PŘEHRÁVAČ · DŽUBOX · ATARIHELP.EU ·
  HELP**. MP3 = Retro Cassette Player, DŽUBOX = PRO DJ MIXER, ATARIHELP.EU je karta před HELP (jak jsi
  vybral). OPTIONS = kolečko vpravo nahoře.
- **Prst:** karty jedou přesně za prstem. Rychlý švih přeskočí víc karet, prst položený na jedoucí karty
  je zastaví. Pustíš – karta dojede doprostřed (pružina bez přeskakování).
- **Otevření:** klepnutí na prostřední kartu nebo na tlačítko pod ní (SPUSTIT, PŘEHRÁT, ROZJET,
  OTEVŘÍT WEB, OTEVŘÍT NÁVOD). Karta se rozzáří, přiblíží a teprve pak se přepne stránka.
  Klepnutí na kartu vedle nebo na tečku = dojede doprostřed.
- **Klávesnice, ovladač, dálkový ovladač:** šipky (i L1 / R1), Enter / OK / A / mezerník, Home / End.
- **Nulová odezva:** kreslí C++ přes OpenGL ES (GLES 3, na starších telefonech GLES 2) na vlastním
  vlákně. Dotyk jde z Androidu rovnou do C++ bez čekání na snímek a C++ sám rozhodne, co klepnutí
  znamená – Java jen přepne stránku. Když telefon nestíhá, menu samo sníží náročnost (úroveň 0–3).
  V logu je to řádek `B311 MENU snimek … ms … zmeskano … uroven …`.
- **Zamčené SEGA a PS1:** karta je šedá se zámkem a nápisem ZAMČENO. Klepnutí na ni = karta se
  zatřese a pod kartami se ukáže, co chybí („Napiš v Atari ještě 45 řádků programu …“,
  „Hraj na Seze ještě 12 min.“).
- **Na šířku** je vidět víc karet vedle sebe.
- **Pojistka:** kdyby C++ menu na nějakém telefonu nešlo (chyba OpenGL, nepřipraví se do 8 s), appka
  sama ukáže stejné menu v HTML (karusel z týchž karet, se stejnými zámky).
- Po úvodním filmu karty přiletí, po návratu z emulátoru se menu jen rychle prolne.

## 2. Živé karty (kreslí je C++ v reálném čase)

| karta | co je na ní |
|---|---|
| ATARI 130XE | 3D model Atari 130XE (z úvodního filmu), pomalu se natáčí ve světle |
| SEGA MEGA DRIVE | 3D konzole N&P 16 s kazetou a neonovými světly |
| PS1 | 3D konzole N&P 32 s CD |
| MP3 PŘEHRÁVAČ | 3D kazeta N&P C-90, cívky se točí rychlostí skutečného kazeťáku |
| DŽUBOX | neonový jukebox: 16pásmový ekvalizér tančí do rytmu 124 BPM, reproduktory pumpují basy, rázové vlny, světla klubu, noty |
| ATARIHELP.EU | zeměkoule z teček, kolem obíhá mince N&P |
| HELP | 3D kniha NÁVOD, listuje se v ní stránka, nad ní zlatý otazník |

Ostatní karty mohou mít jen obrázek (viz bod 3).

## 3. Nová karta = obrázek + jeden řádek

Všechno je v `assets/menu/karty.txt` (návod je přímo v souboru):

```
id | NÁZEV | podtitul | popis | obrázek | cíl | barva | zámek | efekt | tlačítko
```

- obrázek 600 × 800 (png / jpg) do `assets/menu/`,
- cíl = stránka v appce, web (`https://…`), nebo `akce:nastaveni` (OPTIONS) / `akce:intro` (úvodní film),
- zámek `sega` / `ps1` = otevře se postupem hráče, prázdné = vždy otevřená,
- **efekt** rozhoduje, jestli je karta živá (`3d:atari`, `3d:nap16`, `3d:nap32`, `fx:kazeta`, `fx:dzubox`,
  `fx:web`, `fx:help`), nebo jen obrázek (prázdné).

Posouvání, tečky, klávesy i obraz na TV se srovnají samy – v kódu se nic nemění. Ověřeno s 1 i 15 kartami.
Chybný řádek menu nezastaví (zapíše se do logu), a kdyby soubor chyběl úplně, má C++ v sobě zálohu
těch sedmi karet.

## 4. HELP = úplný návod

`assets/napoveda/index.html` – zlato-černý jako menu, česky, 13 kapitol: hlavní menu, Atari 130XE, Sega,
PS1, MP3 přehrávač, DŽUBOX, ATARIHELP.EU, OPTIONS, WEB TV a PC, odemykání, soubory v telefonu, log a
hlášení chyby, časté otázky. Navrchu rychlý start a obsah, **hledání** (nevadí háčky ani koncovky:
„sipky“ najde „šipky“ i „šipkami“), na konci každé kapitoly OBSAH a ZPĚT DO MENU. Posun po návodu se
nezapisuje do historie, takže ZPĚT na telefonu z návodu vrací rovnou do menu.

Všechno v návodu je opsané z kódu appky. Každý název tlačítka v návodu (147 různých) jsem strojově
porovnal s appkou – existuje přesně tak, jak ho člověk uvidí. Jeden nesoulad jsem tak našel a opravil
(položky ATR / DISK).

## 5. Menu na TV / PC

Na TV jde menu proudem z C++ stejně jako úvodní film (`/menu.stream`, 854 × 480). Klávesnice počítače
menu ovládá: šipky nebo A / D listují, Enter / mezerník otevře kartu, Home / End. C++ v obraze ukazuje
„ŠIPKAMI LISTUJ · ENTER OTEVŘE“.

## 6. Další opravy

- **OPTIONS → SPUSTIT WEB TV:** dřív se na pár vteřin ukázala celá odpověď Javy
  („TV_WEB_CAST_OK http://… APP“). Teď zůstane pod tlačítkem jen adresa (velkým písmem), tlačítko
  píše WEB TV BĚŽÍ a adresa je vidět i při každém dalším otevření OPTIONS.
- **ZPĚT v hlavním menu:** dřív skočil podle historie stránek zpátky do posledního emulátoru
  (menu → Sega → MENU → ZPĚT = zase Sega). Teď appka jde do pozadí jako tlačítkem plochy a po
  návratu je menu na stejné kartě. V záložním HTML menu ZPĚT napřed zavře otevřené OPTIONS.
- **DŽUBOX – RECORD:** nahrávka mixu se v appce **nikam neukládala** (WebView neumí stáhnout „blob“, appka
  ho zahodila). Teď jde po kouscích do Javy a uloží se do `Download/AtariHelp/DJ_mixy`
  (`NP_VISION_mix_<datum>_<čas>.webm`), na tlačítku SAVING … % a SAVED ✓. V prohlížeči na PC se stáhne
  jako dřív.
- **Písmo Chakra Petch** je teď v appce i pro stránky (OPTIONS, návod, záložní menu). Dřív ho HTML
  nikdy nenačetlo a telefon ukazoval náhradní písmo. +34 kB.
- **Smazán obrázek starého menu** `nap_ui/nap_vision_intro_screen1.png` (3,6 MB), nic na něj nevede.
- Karta ATARIHELP.EU: „hry pro Atari, Segu i PS1 … hra z webu se spustí rovnou v emulátoru“ – ověřeno,
  appka posílá hru z webu do správného emulátoru.

## 7. Co jsem na PC ověřit nemohl – prosím na telefonu

- **Plynulost na skutečném GPU.** Na PC kreslí Mesa procesorem (~130 ms na snímek 1080 × 2340), to nic
  neříká o telefonu. Po spuštění pošli log – řádek `B311 MENU snimek … ms, zmeskano …, uroven …`.
- Vrstvení C++ menu nad stránkou na tvém telefonu, ZPĚT v menu a zápis nahrávky DŽUBOXU do Download
  (Java je přeložená proti skutečnému Androidu API 34 bez jediné chyby, ale běží jen na telefonu).

## 8. Na co jsem narazil – rozhodni ty

1. **ODEMKNOUT DALŠÍ a ZAMKNOUT VŠE** (tvoje zadní vrátka) jsou v OPTIONS vidět pro každého, proto je
   návod popisuje. Mám je před vydáním 20. 11. schovat (a vyhodit z návodu)?
2. **Zámky hlídá jen menu.** Hra pro Segu nebo PS1 klepnutá na webu atarihelp.eu se spustí, i když je
   Sega / PS1 zamčená. Mám to zavřít?
3. **Kazeťák (MP3):** tlačítka RECORD a FAVORITES nic nedělají, v návodu proto nejsou. Doplnit, nebo schovat?
4. V appce leží **asi 11 MB obrázků, na které nic nevede** (`assets/design/*.png` kromě loga N&P,
   `img/logo.png`, `img/pecet.png`). Smazat?

## Ověřeno na PC

- **Logika karuselu (C++):** 61/0 – karty.txt i s chybnými řádky, fyzika prstu a švihu, rozložení
  na výšku / na šířku / TV, klávesy, 1 a 15 karet.
- **Skutečný `nap_menu_gl.cpp` jako z Javy (JNI):** 14/0 – karty, kreslicí vlákno, tah prstem, švih,
  zamčená SEGA, kolečko při jiné velikosti plochy, klávesy, spuštění karty, proud na TV 854 × 480,
  snímek pro starý prohlížeč, plocha pryč a zpět (návrat z emulátoru) na stejné kartě.
- **ThreadSanitizer:** 0 závodů v našem kódu (22 hlášení jsou uvnitř Mesy na PC, na telefonu Mesa není).
- **Shadery:** 17 shaderů a 20 programů menu přísně podle OpenGL ES 2.0 (referenční překladač
  Khronos) – 0 chyb. Mesa na PC je shovívavější než telefony, tohle hlídá Mali / Adreno / PowerVR.
- **Snímky menu** (Mesa): na výšku, zamčené, na šířku, TV – 0 chyb OpenGL.
- **Menu v Chromiu** (přesně soubor z appky): 24/0 – záložní karusel, zámky, HELP → návod, pod C++
  menu, OPTIONS, adresa WEB TV, stránka Atari 130XE s novými body testu.
- **Návod v Chromiu:** 23/0 – obsah, kapitoly, obrázky, písmo, hledání, historie, šířka 320 / 360 px
  a na šířku; **názvy tlačítek proti appce:** 147/0.
- **DŽUBOX RECORD v Chromiu:** 11/0 – nahrávka 24 s po kouscích, složený soubor jde přehrát (Opus,
  24,9 s), chyba zápisu, PC.
- **Menu na TV / PC v Chromiu:** 9/0 – proud ze skutečného C++ menu, obraz běží, šipky / A / D / End /
  Home z PC listují, Enter na zamčené SEGE nic nespustí, Enter na ATARI ji spustí.
- **Java** přeložená proti skutečnému Androidu (android.jar API 34, Java 17 jako v build.gradle): 0 chyb.
- **Podpisy JNI:** všech 22 funkcí mostu Java ↔ C++ souhlasí (jméno, návratový typ, typy parametrů)
  a všechny jsou z knihovny `napmenu` vidět.
- **Mosty JavaScript → Java:** 101 volání, všechna mají metodu v Javě. **Odkazy:** 83 v HTML, 8 ve
  stylech, 14 cest v Javě a C++, 13 souborů z karty.txt – nic nechybí.
- **Regrese TV / PC se stránkou z B311:** klávesnice PC 23/0, Atari na TV + Ctrl+V 23/0. (Při jednom
  běhu celé sady naráz neprošla v testu klávesnice PC jedna kontrola opakování držené klávesy – ta měří
  časování. Samotný test pak 3× bez chyby; změny B311 v prohlížeči posílání kláves Atari nemění.)

Postup: `test_overeni/b311/JAK_SPUSTIT.txt`, výstupy `test_overeni/b311/VYSTUP_*`.
