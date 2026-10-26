# Co je v B316

Tvoje zpráva: „Šlo by do Atari emu přidat i příponu *.car – tedy aby tyto hry šly v našem emu
spustit?“ (poslal jsi Wonderboy_VBXE.car)

## 1. Co umí

- **Soubory .car (cartridge)** pro Atari 130XE – celé v C++, jako skutečný slot pro cartridge:
  - z telefonu přes **XEX / MOBIL**,
  - z webu přes **NET / HRY** (odkaz na .car),
  - i zabalené v **ZIP**.
- Appka pozná cartridge podle obsahu (hlavička „CART“), ne jen podle přípony.
- Po výběru se cartridge zasune do slotu a Atari se s ní zapne. Je to jako skutečná cartridge:
  - přednost před BASICem,
  - přepínání bank,
  - TRIG3, podle kterého OS pozná, že je cartridge zasunutá.
- **Tvůj Wonderboy_VBXE.car**:
  - typ 42 (Atarimax 1 MB, 128 bank po 8 kB, kontrolní součet v pořádku), hra pro VBXE;
  - na PC v našem jádře naběhne titulní obrázek WONDER BOY;
  - po START a FIRE hra: les, Wonder Boy běží, skóre, energie;
  - za první minutu (titulka a hra) se banky přepnuly 24 673× a VBXE nakreslilo 9 304 bloků obrazu (blitter).
- **Cartridge zůstává ve slotu** (jako disketa v D1:):
  - i po vypnutí a zapnutí POWER,
  - i po novém otevření Atari z menu – hra pak naběhne znovu.
- **Jak cartridge vytáhnout:**
  - **XEX / MOBIL** – když je cartridge ve slotu, nabídne:
    - **VYSUNOUT CARTRIDGE** – Atari se zapne znovu s BASICem,
    - **VYBRAT JINÝ SOUBOR**;
  - jinak ji vytáhne sám každý jiný program: XEX, NET hra, ATR disketa, TXT do BASICu, kazeta s hrou.
  - Cartridge zase vysune disketu z D1:, aby nenabootovala místo ní.
- **67 typů cartridge** – všechno, co pro 130XE umí emulátor Atari800, kromě několika vzácných:
  - Standard 2–16 kB,
  - XEGS a Switchable XEGS do 1 MB,
  - MegaCart do 4 MB,
  - Atarimax 128 kB / 1 MB,
  - SIC!,
  - Williams, Express, Diamond,
  - SpartaDOS X,
  - OSS, Blizzard, Phoenix,
  - Turbosoft, Atrax, Bounty Bob,
  - J(atari)Cart, DCart …
- **Špatný soubor = poctivá hláška** na přístroji i v logu. Atari přitom běží dál, rozepsaný program
  zůstane. Hlášky:
  - NENÍ .CAR,
  - CARTRIDGE PRO ATARI 5200,
  - JEN PRO PRAVÝ SLOT ATARI 800,
  - NEZNÁMÝ TYP,
  - TYP n ZATÍM NEUMÍM,
  - SOUBOR .CAR JE NEÚPLNÝ.
- **Poctivě, co zatím ne:**
  - Hra si do flash paměti cartridge nic neuloží – stejně jako v emulátoru Atari800.
  - Typy AST 32 kB, XEGS 8F, The!Cart (32–128 MB), Ram-Cart a SiDiCar zatím neumím – appka to
    řekne.
  - Soubory .rom / .bin bez hlavičky CART zatím ne.
- **Do logu:**
  - „B316 CARTRIDGE VLOZENA …“ (typ, velikost, součet, banka),
  - co cartridge vytáhlo,
  - v LOG / CHYBA stav cartridge (banka, okna, TRIG3, kolikrát se přepnula).

## Co prosím vyzkoušet

1. Nainstaluj B316 (postup zůstává).
2. ATARI 130XE → **XEX / MOBIL** → vyber **Wonderboy_VBXE.car** – titulní obrázek.
3. START a FIRE (na šířku tlačítka, nebo ovladač) → hraj.
4. POWER vypnout / zapnout → hra naběhne znovu.
5. **XEX / MOBIL** → **VYSUNOUT CARTRIDGE** → Atari s BASICem (READY).
6. Když chceš, totéž na TV / PC (obraz a zvuk jdou jako u jiných her).
7. LOG / CHYBA → pošli log (řádky „B316 CARTRIDGE …“).

## Ověřeno na PC

- **Naše cartridge proti emulátoru Atari800** (jeho zdrojový kód cartridge, přeložený vedle našeho):
  - 67 typů × 6 000 náhodných přepnutí;
  - po každém kroku se porovná, co procesor vidí (okna $8000–$BFFF, všech 16 kB, TRIG3, čtení
    $D5xx): **shoda ve všech krocích**;
  - tvůj soubor 30 000 kroků: shoda;
  - načtení .car (typy 0–170, špatný součet, krátký soubor): shoda;
  - jediná záměrná odchylka: neobsazená čísla typů 113–158 – Atari800 je vezme jako prázdnou
    cartridge, my řekneme „neznámý typ“;
  - test opravdu chyby najde: 6 schválně podstrčených chyb – našel všech 6.
- **Skutečné jádro 130XE s OS** (malé cartridge si test sám sestaví) – **50/0**:
  - OS cartridge spustí;
  - ANTIC kreslí přímo z ní;
  - Atarimax: banka 127 → banka 0 → vypnutí → paměť RAM pod cartridge nepoškozená → zase zapnutí;
  - RESET banku nechá, POWER ji vrátí;
  - Bounty Bob;
  - XEGS;
  - Wonderboy 3000 snímků (titulka, hra, VBXE, obraz).
- **Skutečný C++ přístroje s vlákny jako v telefonu** – **45/0**:
  - špatné soubory,
  - POWER,
  - VYSUNOUT,
  - XEX / ATR / TXT / kazeta cartridge vytáhnou,
  - Wonderboy až do hry,
  - obraz pro TV.
- **Regrese:**
  - Atari bez cartridge se chová **do bajtu stejně jako B315** – všechny režimy testů jádra
    (BASIC, kazeta, TXT, Turbo-BASIC, disketa, klávesnice PC, proud pro TV, Acid800) i snímky
    obrazu 10 her a programů (Popeye a Wolfenstein přes VBXE …);
  - testy přístroje z B291 a B292–B303.
- **Java** proti skutečnému Androidu: 0 chyb. Všech 41 propojení Java ↔ C++ sedí.
- **Návod:** změnily se jen odstavce XEX / MOBIL a NET / HRY (česky i anglicky); názvy tlačítek
  v návodu v appce opravdu jsou.
- **PS1 myš pro Dune 2000** zůstává na další verzi (jak jsi chtěl – jedno po druhém).
