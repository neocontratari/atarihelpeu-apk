# Co je v B312

Tvoje odpovědi k B311: „1 – tlačítka schovat, ale nějak konstruktivně – třeba pod kód, který bude
vědět jen já. 2 – samozřejmě uzamknout. 3 – tlačítka doplnit, rozhodně neschovávat, a dopsat do
nápovědy. 4 – klidně vymaž, pokud nejsou potřeba.“

## 1. ODEMKNOUT DALŠÍ a ZAMKNOUT VŠE – schované pod servisním kódem

- V OPTIONS už tlačítka nejsou vidět a návod je nepopisuje. Objeví se až po **servisním kódu**,
  který znáš jen ty – jak servis otevřít a kód máš v mé zprávě (do repozitáře ani do appky se kód
  nedostal, je veřejný).
- Správný kód otevře servis na **10 minut** (OPTIONS ukazuje „Servis je otevřený ještě … min.“).
  Pak se zase sám zavře.
- Špatný kód: „Špatný kód.“ Po 3 špatných pokusech minuta pauza.
- Hlídá to **Java**, ne jen stránka: tlačítka by šla zavolat z jakékoli stránky ve WebView (i z webu) –
  bez otevřeného servisu je Java odmítne. V appce je jen otisk kódu (PBKDF2-HMAC-SHA256, 60 000×,
  náhodná sůl), kód samotný ne. Napsaný kód se nikam nezapisuje, ani do logu.

## 2. Zámky SEGA a PS1 platí všude

- Dřív hlídalo zámky jen hlavní menu: hra pro Segu nebo PS1 klepnutá na webu atarihelp.eu se spustila,
  i když byla konzole zamčená. Teď ji appka **ani nestáhne** a dole napíše proč – stejnými slovy
  jako zamčená karta v menu, třeba **„SEGA JE ZAMČENÁ – Napiš v Atari ještě 37 řádků programu …“**
  nebo **„PS1 JE ZAMČENÁ – Hraj na Seze ještě 12 min.“**
- Hlídané jsou všechny cesty: odkaz na webu, stažený soubor, ZIP se hrou Segy (i když se stahoval jako
  hra pro Atari), odkaz na Google Drive pro PS1, výběr hry PS1 z telefonu.
- Stránka emulátoru zamčené konzole se neotevře ani jinou cestou (staré odkazy, historie) – appka se
  vrátí do hlavního menu.
- Hry pro Atari jdou z webu jako dřív. V logu je každé zastavení řádek `B312 ZAMEK …`.

## 3. MP3 přehrávač – RECORD a FAVORITES fungují

**RECORD = vlastní mixtape**
- Nahrává, co přehrávač hraje – **i s nastavením EQ, BASS, TREBLE a BALANCE**. Knoflík VOLUME nahrávku
  neztlumí.
- Když nic nehraje, přehrávání se rozjede samo (jako na kazeťáku). Tlačítko RECORD jemně červeně svítí,
  dole běží **● REC 00:42 • skladba**.
- **PAUSE** nahrávání pozastaví (❚❚ REC), PLAY pokračuje. Pauza ani načítání další skladby se
  nenahrávají – skladby jdou v nahrávce rovnou za sebou.
- Konec a uložení: RECORD znovu, STOP, konec playlistu, MENU nebo ZPĚT na telefonu (appka nejdřív
  uloží, pak teprve odejde – dřív by se nahrávka zahodila). Nejdéle 90 minut, jako kazeta C90.
- Uloží se do **`Download/AtariHelp/Mixtape`** jako `NP_VISION_mixtape_<datum>_<čas>.m4a` (na starším
  WebView `.webm`) a hned je v seznamu hudby.

**FAVORITES = oblíbené**
- Hvězdička ☆ u skladby (v playlistu i v seznamu hudby v telefonu) ji přidá do oblíbených (★).
- FAVORITES dole otevře oblíbené: klepnutí skladbu pustí, ✕ ji odebere (napřed se zeptá ODEBRAT?),
  ★ PRIDAT / ☆ ODEBRAT HRAJICI SKLADBU, **▶ HRAT VSECHNY OBLIBENE**.
- Appka si oblíbené pamatuje i po vypnutí. Když soubor v telefonu přesuneš, najde ho podle jména.

**Opravy v přehrávači, na které jsem přitom narazil**
- **VOLUME se uplatňoval dvakrát:** jakmile jsi sáhl na ekvalizér, 50 % hrálo jako 25 % (ověřeno
  v Chromiu: hlasitost přehrávače zeslabuje i zvuk, který jde do ekvalizéru). Teď jednou.
- Seznam hudby ukazuje i **M4A, AAC, OGG, OPUS, FLAC a WebM** (dřív jen MP3 a WAV; M4A se přejmenovalo
  na „audio_123.mp3“, nahrávky DŽUBOXU ve WebM v seznamu vůbec nebyly).
- Každá skladba z telefonu zůstávala v paměti celá i po přepnutí na další – dlouhý playlist mohl
  telefon zahltit. Teď v paměti zůstává jen ta, která hraje. Stejná skladba se do playlistu znovu
  nepřidává.
- ZPĚT na telefonu nejdřív zavře otevřený panel (playlist, FAVORITES, SETTINGS).

## 4. DŽUBOX – RECORD

- Nahrává **M4A**, když to WebView umí (přehraje ho každý přehrávač). Na starším WebView WebM – a ten má
  teď v hlavičce **délku** (dřív ne: přehrávače neukázaly délku a špatně posouvaly).
- Když z pultu odejdeš během nahrávání (MENU, ⏻, ZPĚT), mix se nejdřív uloží.
- Přehrávač i DŽUBOX ukládají nahrávky jedním společným kódem (`assets/spolecne/nap_nahravka.js`).

## 5. Smazané nepoužité obrázky – 13,8 MB

`assets/design/` (6 obrázků, logo N&P zůstalo), `img/logo.png`, `img/pecet.png` a
`nap_ui/nap_intro_command_center.png` (3,2 MB – patřil ke staré, nikde nepoužívané obrazovce;
smazal jsem i tu mrtvou třídu `NapExactImageScreens`). Nic na ně nevedlo – strojově ověřeno.

## 6. Návod (HELP)

MP3 přehrávač: nové bloky **FAVORITES** a **RECORD – vlastní mixtape**, tip „mixtape z oblíbených“.
DŽUBOX: M4A a uložení při odchodu. ATARIHELP.EU a Odemykání: zámky platí i pro hry z webu. Soubory:
složka Mixtape. Dvě nové časté otázky. Z OPTIONS zmizela servisní tlačítka. Všech 162 názvů tlačítek
v návodu strojově porovnaných s appkou.

## 7. Co jsem na PC ověřit nemohl – prosím na telefonu

- Formát nahrávky: M4A s AAC dělá jen WebView s kodekem AAC (Chrome na telefonu ho obvykle má). Bez něj
  appka sama vezme M4A s Opusem nebo WebM. Na PC (Chromium bez AAC) vznikalo M4A s Opusem.
- Zápis nahrávky do `Download/AtariHelp/Mixtape` přes MediaStore a hláška zámku (Toast) dole na displeji.
- Servisní kód na tvém telefonu (ověření trvá zlomek vteřiny – na PC 0,1 s).
- Kroky testu jsou v appce: ATARI 130XE → pod přístrojem.

## Ověřeno na PC

- **Java bez Androidu (JVM):** 14/0 – PBKDF2 = Python hashlib (45 vektorů i z RFC), skutečný servisní kód
  proti otisku v appce (kód jen v proměnné, nikam se nevypsal), malá písmena a pomlčka nevadí, žádný kód
  s jedním jiným znakem neprojde, 3 špatné pokusy = minuta pauza, servis 10 minut; která stránka je
  SEGA / PS1, důvod zámku (1 řádek / 3 řádky / 45 řádků), hudba v seznamu, ZPĚT.
- **Zdroják MainActivity:** 18/0 – každá cesta ke hře Segy / PS1 se nejdřív zeptá na zámek (dřív, než
  cokoli stáhne), stránka zamčené konzole se vůbec nenačte, všechny cesty z webu vedou jen přes hlídané
  metody, servisní tlačítka bez servisu nic neudělají.
- **MP3 přehrávač v Chromiu** (přesně soubor z appky, Java napodobená): 30/0 – seznam hudby, uvolňování
  paměti, bez duplikátů, oblíbené i po novém spuštění, HRAT VSECHNY, přesunutý a chybějící soubor;
  RECORD: nahrávka bez pauzy (4,5 s = jen hraní), M4A, **VOLUME 30 % → nahrávka stejně silná**
  (poměr 1,00), **BALANCE vlevo → pravý kanál o 30 dB tišší** (nahrává se za efekty), dvě skladby
  za sebou bez mezery, uložení při MENU / ZPĚT, WebM s délkou, chyba zápisu, PC.
- **DŽUBOX v Chromiu:** 12/0 – M4A (délka v hlavičce = délka zvuku), WebM s doplněnou délkou, chyba
  zápisu, PC, odchod během nahrávání.
- **OPTIONS v Chromiu:** 18/0 – bez kódu žádná servisní tlačítka, špatný kód, pauza, správný kód,
  ODEMKNOUT / ZAMKNOUT, 10 minut, kód se nedostane do logu; stránka Atari 130XE s kroky B312.
- **Návod v Chromiu:** 26/0, **názvy tlačítek proti appce:** 162/0.
- **Mosty JavaScript → Java:** 103/0. **Odkazy:** 0 chyb, smazané obrázky nikde.
- **Java proti skutečnému Androidu** (android.jar API 34): 0 chyb.
- **Regrese hlavního menu B311** v Chromiu: 23/24 – jediná „chyba“ je záměrná (test B311 čekal kroky
  testu B311, teď jsou tam kroky B312).
- **Servisní kód v repozitáři:** 0 nálezů (soubory i historie).
- C++ se v B312 neměnilo.

Postup: `test_overeni/b312/JAK_SPUSTIT.txt`, výstupy `test_overeni/b312/VYSTUP_*`.
