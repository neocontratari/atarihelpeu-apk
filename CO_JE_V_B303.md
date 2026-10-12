# Co je v B303

Tvoje zpráva: „Funguje to – ale má to na TV zpoždění, nejde to vylepšit?
Chtělo by to menší lagy. Opravdu převod jede jak má v C++? … Šlo by i
vkládat kód do okna monitoru PC?“

## Upřímná odpověď: ne, převod v C++ nejel

Do B302 dalo C++ jen hotový snímek. Pak ho **Java** zvětšovala na 1280×720 a
komprimovala (H.264 nebo JPEG) na hlavním vlákně appky. V tvém logu: snímek
jen každých ~37 ms, JPEG 27–33 ms na snímek (asi 14 snímků/s) a prohlížeč u
H.264 ještě sám bufferoval. Odtud zpoždění na TV.

## B303: přenos na TV / PC celý v C++

- **Jak:** stejně jako dřív – zapni přenos na TV / PC a otevři HELP (Atari
  130XE). Po aktualizaci appky stránku v prohlížeči na PC / TV obnov (F5).
- C++ zabalí **každý snímek Atari (50 za vteřinu)** jako paletu barev +
  jen řádky, které se změnily, a přidá k němu zvuk. Java už nic nezvětšuje
  ani nekomprimuje – jen pošle bajty (`/atari.stream`).
- Prohlížeč to rozbalí do obrazu (canvas) a zvětší sám, se správným poměrem
  stran Atari. Zvuk jde ve stejném proudu, polštář se přizpůsobí síti
  (50 ms na rychlé síti, až 300 ms, když síť zlobí) – zpoždění zvuku neroste.
- Obraz je **přesně** obraz z jádra (žádná komprese, žádné rozmazání).
  Posílá se 0,7–1,5 Mbit/s (H.264 předtím ~27 Mbit/s).
- Během přenosu drží appka **Wi-Fi telefonu v režimu nízké latence**
  (úsporný režim Wi-Fi jinak zdržuje klávesy z PC do telefonu o desítky až
  stovky ms). Proto nové oprávnění WAKE_LOCK – běžné, nic nepotvrzuješ.
- Když prohlížeč proud neumí, nebo nejsi v Atari 130XE (HELP), jede to
  postaru (MJPEG / H.264) – starý ATARI 130XE EMULATOR, PS1 a SEGA beze změny.

### Naměřeno na PC (Chromium, stránka přímo z appky, skutečné jádro)

| | |
|---|---|
| snímků v prohlížeči | 50 / s (každý snímek Atari), 0 chyb |
| obraz v prohlížeči proti jádru | 0 rozdílných bodů ze 161 280 |
| zvuk | 44 100 vzorků/s, 0 podtečení, polštář 50 ms |
| **klávesa na PC → písmeno v obraze prohlížeče** | **medián 41 ms, nejvíc 46 ms** (dřívější běh 42 / 53 ms) |

Z toho: prohlížeč → server 3–7 ms, server → obraz v prohlížeči 1–5 ms, a
26–41 ms samotné Atari: emulace jede po snímcích (50× za vteřinu), klávesa
se převezme na začátku dalšího snímku a písmeno je vidět, až ANTIC nakreslí
jeho řádek – často až v následujícím snímku. Skutečné 130XE to má podobně
(do 1–2 snímků, snímek = 20 ms).

**Na TV navíc:** Wi-Fi a hlavně samotná televize – bez herního režimu si
obraz ještě 50–150 ms zpracovává. **Zapni na TV „Herní režim“ (Game mode)**,
pokud ho má. Na skutečném telefonu a TV jsem to změřit nemohl.

## Vkládání kódu z PC (Ctrl+V)

- V okně prohlížeče na PC (Atari 130XE z HELP na obrazovce telefonu)
  **Ctrl+V** (na Macu Cmd+V) = vloží, co máš ve schránce. Nebo **přetáhni
  TXT soubor** do okna.
- **Řádky s číslem** (program) napíše appka do ATARI BASICu / Turbo-BASICu
  rychle a přesně (jako TXT v B299), ale **bez NEW** – co v Atari je,
  zůstane; řádek se stejným číslem se přepíše. Nahoře v okně se ukáže
  `VLOZENO: 5 RADKU PROGRAMU (BEZ NEW) - PISU`.
- Jiný text (bez čísel řádků, např. `RUN` nebo odpověď na INPUT) se napíše
  jako klávesy, asi 15 znaků za vteřinu.
- Česky: ě š č ř ž ý á í é ú ů → bez háčků a čárek („Příšerně“ → „Priserne“).
- Píše se, až Atari čeká na řádek (READY nebo hned po zadaném řádku).
  Když zrovna běží program, appka počká (BREAK) – nejvýš 45 s.
- XEX / ATR / BAS (SAVE) přetažený omylem se nepíše:
  `VLOZENI: TO NENI TEXT`. Vypnuté Atari: `VLOZENI: ATARI JE VYPNUTE`.

## Co jsem při testech našel a opravil

1. **TXT s „ě“ se rozsypal (od B299, i soubor z telefonu).** „ě“ je v UTF-8
   bajty C4 9B – a bajt $9B je v ATASCII konec řádku. Appka pak brala celý
   soubor jako výpis z Atari a řádek s „ě“ rozsekla: ze
   `30 ? "Příšerně žluťoučký kůň"` zbylo `30 ? "PE.C-E.rnD` a další řádek
   chyběl. Teď: platné UTF-8 = text z PC, ATASCII se pozná jen u výpisu z Atari.
2. **Druhé vložení hned po prvním by čekalo 45 s a vzdalo to.** Po zadaném
   řádku programu BASIC READY nenapíše. Appka teď pozná, že editor čeká na
   nový řádek (kurzor na začátku prázdného řádku, procesor čeká na klávesu),
   i bez READY. Běžící program to není (ten čeká na BREAK).
3. **Log se zahlcoval.** `DARK_FRAME_SPIKE` byl v tvém logu 350× z 1030 řádků
   a vytlačil všechno ostatní. Teď nejvýš jednou za 10 s, průměry TV
   jednou za ~15 s, stav TV jednou za 30 s.

## V logu

- `B303 TV PROUD: prohlizec N pripojen …`, každých 30 s `… snimku/s,
  Mbit/s`, při odpojení souhrn.
- `B303 PROHLIZEC: 50 snimku/s, zvuk hraje, napred 52 ms …` – co hlásí
  prohlížeč (každých 30 s). „zvuk NEHRAJE – klikni do okna“ = prohlížeč
  ještě nepovolil zvuk (klikni nebo zmáčkni klávesu v okně).
- `B303 WIFI: rezim nizke latence ZAPNUT (LOW_LATENCY)`.
- `B303 VLOZENI z PC: …` a `B299 TXT PROGRAM …` (co se vkládalo a psalo).

## Ověřeno na počítači

- `test_core tv-proud`: 12 kontrol, 0 chyb – BASIC, LIST, 5 her vč. VBXE,
  po rozbalení přesně obraz i zvuk, plná paleta, 600 barev, POWER vyp.
- `test_core vlozeni`: 11 kontrol, 0 chyb (UTF-8 / ATASCII, binárky, bez
  NEW, druhé vložení, běžící program).
- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: 86 kontrol, 0 chyb
  (10 nových: proud 50/s přesně, zvuk, klávesa → proud medián 10 ms,
  vložení programu / textu / binárky, POWER vyp = černá).
- Chromium + stránka přímo z `MainActivity.java` + skutečné jádro:
  23 kontrol, 0 chyb (viz tabulka; i Ctrl+V skutečně z klávesnice,
  přetažený TXT, spadlé spojení, návrat na MJPEG, žádná chyba JavaScriptu).
- Testy z B299 (TXT do BASICu a TBXL) a B302 (klávesnice) dávají stejné
  výsledky jako předtím.
