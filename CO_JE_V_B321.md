# Co je v B321

Tvoje zpráva: „Chci mít apku ošetřenou tak, aby ji nikdo nemohl přeposlat a aby stažení her fungovalo.
Máme 2 možnosti – buď můj server, nebo Wedos. Od tebe čekám řešení.“

Řešení: **hry jdou z tvého serveru, appku chrání osobní kódy.** WEDOS zůstává jen pro web.

## 1. Hry z tvého serveru – N&P HRY

- **Atari 130XE → NET / HRY, Sega → SBIRKA, PS1 → LOAD GAME** teď otevřou **N&P HRY**: hry z tvého
  PC (složky Arcade\Atari, Arcade\Sega, Arcade\PS1) s obaly, nahoře ATARI / SEGA / PS1 a hledání.
- **Klepneš na hru → stáhne se z tvého serveru a spustí ve správném emulátoru** (stejnou cestou jako
  dřív hry z webu). PS1 ZIP se rozbalí do knihovny PS1 a příště se spustí rovnou z telefonu.
- **Bez internetu** N&P HRY ukážou naposledy načtený seznam. Stahovat jde, až bude server zase dostupný
  (seznam obnoví tlačítko ↻).
- **Web atarihelp.eu** (karta v menu a tlačítka WEB) se otevře **v prohlížeči telefonu**. V okně appky ho
  WEDOS odmítal (to byla ta chyba 503), v Chromu projde.
- Zámky Segy a PS1 platí dál i pro hry ze serveru.

## 2. Appku nikdo nepřepošle

- **Při prvním spuštění se appka aktivuje.** Když ji člověk stáhl se svým kódem ze stránky
  `http://46.33.114.139:8080/aktivace`, server do ní vložil jeho osobní klíč a appka se spáruje sama.
  Jinak napíše kód `NAP-XXXX-XXXX`, který mu dáš.
- **Jeden kód = jeden telefon.** Přeposlaná appka na jiném telefonu nefunguje. Napíše „Tahle kopie
  aplikace patří jinému telefonu“ a v okně serveru uvidíš **„🚫 PŘEPOSLANÁ APPKA“** i s modelem telefonu.
- **ZABLOKOVAT** v okně serveru appku na tom telefonu zamkne. Po **ODBLOKOVAT** stačí v appce klepnout
  na ZKUSIT ZNOVU.
- **Nový telefon:** v okně serveru **UVOLNIT**. Starý telefon se odpojí a kód jde zadat znovu.
- Každý dotaz appky na server je podepsaný tajným klíčem telefonu. Bez spárování server nic nedá:
  ani seznam her, ani hru.
- Appka už není „ladicí“. Přes kabel z PC (adb) se z ní nedají vytáhnout data ani klíč telefonu.

## 3. Nový podpisový klíč – bez přeinstalace

- GitHub teď appku podepíše **tvým novým soukromým klíčem** s „rotací klíče“. Android 9 a novější pak
  věří jen novému klíči: se starým veřejným klíčem už nikdo nepodstrčí falešnou aktualizaci.
- **B321 se nainstaluje jako normální aktualizace přes B320** – uložené hry i nastavení zůstanou.
- Potřebuju od tebe tajemství **NAP_PODPIS** v GitHubu (návod JAK_NASTAVIT_PODPIS.txt). Dokud tam není,
  GitHub appku podepíše jen starým klíčem a v buildu napíše varování.

## 4. Aktualizace z tvého serveru

- Appka se tvého serveru zeptá, jakou verzi tam máš (server ji přečte přímo z APK). Když je novější,
  sama nabídne **AKTUALIZOVAT** a nainstaluje ji rovnou ze serveru. Uložené hry zůstanou.
- **Opravená stará chyba:** aktualizace z appky se nikdy nedokončila, protože appka neotevřela okno
  Androidu „Nainstalovat?“. Teď ho otevře.

## 5. Server v10

- NapServer_v10.zip (návod NAVOD_SERVER_v10.txt): kódy, párování, seznam her, stahování s navazováním,
  okno **🔑 KÓDY A TELEFONY**, opravy chyb z v9. Server teď navíc v okně ukáže verzi APK,
  např. „Aplikace načtena: EMU10-B321-SERVER (364)“.

## Jak to rozjet (pořadí)

1. Spusť server v10 místo v9 (NAVOD_SERVER_v10.txt).
2. Vlož tajemství NAP_PODPIS do GitHubu a napiš mi „klíč je tam“. Spustím build a zkontroluju podpis.
3. APK z GitHubu (Build APK → app-debug) ulož jako `C:\ApkaServer\AtariHelp_Vision_v1.apk`.
   V okně serveru musí být „osobní klíč se do APK vkládá ✔️“.
4. Vyrob kód sobě, otevři v telefonu `http://46.33.114.139:8080/aktivace`, zadej kód, stáhni appku
   a nainstaluj. Spáruje se sama.
5. Pak rozdávej kódy ostatním. Na atarihelp.eu dej odkaz na stránku /aktivace.

## Na rovinu

- **Ověřeno na PC:**
  - server: 149 kontrol;
  - třídy appky proti skutečnému serveru: 28 kontrol;
  - stránky aktivace a N&P HRY v Chromiu: 39 kontrol;
  - celá appka přeložená proti Androidu 34;
  - starší testy: oproti B320 žádná nová chyba.
- **Ověřeno v GitHubu na skutečném APK B321:** klíč vložený jako na serveru, podpis pořád platí (apksigner
  od Googlu) a appka si klíč přečte.
- **Na skutečném telefonu to zatím nikdo nezkoušel:** instalace přes B320 s novým klíčem, aktivace
  a stahování z tvého serveru přes internet. To spolu uděláme ve středu.
- **Spojení jde přes http (bez zámečku).** Kdo by odposlouchával síť, viděl by stahované hry. Proti
  přeposílání appky to nevadí. https přes doménu můžeme doplnit později.
- **Kdo má starou B320, musí si B321 stáhnout přes /aktivace se svým kódem.** Stará appka tvůj server
  nezná.
- **Aktivace potřebuje, aby server běžel.** Spárovaný telefon pak hraje i bez serveru, nestahuje jen
  nové hry.
