# Co je v B322

Tvoje zpráva: „Sega se otevře až po 1500 řádcích – jinak donate 2,99 euro, a PS1 se otevře až za
120 hodin hraní Segy – jinak donate 4,99 … ošetřit i to, že když zaplatí, ať se to odemče.
Apka je zdarma, ale pro rychlíky a nedočkavce ne.“

## 1. Nové zámky

- **SEGA** se otevře po **1500 řádcích** programu v Atari (dřív 50).
- **PS1** se otevře po **120 hodinách** hraní Segy (dřív 30 minut). PS1 jde až po Seze.
- Kdo má Segu nebo PS1 otevřenou už teď (z B321), o nic nepřijde.
- Čas Segy se teď přičítá **každou minutu**. Dřív se sezení delší než 6 hodin zahodilo celé.

## 2. ODEMKNOUT HNED – platba

- Zamčená karta má místo ZAMČENO tlačítko **ODEMKNOUT**. Klepnutí otevře obrazovku, kde člověk vidí:
  - **ZDARMA:** kolik ještě chybí (třeba „Napsáno 123 z 1500 řádků“, „Odehráno 3 h 12 min ze 120 h“),
  - **ODEMKNOUT HNED:** **Sega 75 Kč / 2,99 €, PS1 125 Kč / 4,99 €**.
- **QR PLATBA · Kč** je pro české banky, **SEPA · €** pro cizinu (evropský QR kód). QR kód nese
  částku, účet a **variabilní symbol podle kódu** (kód č. 1003 → Sega VS 10031, PS1 VS 10032).
- QR je v telefonu, kterým se platí? **ULOŽIT QR DO GALERIE** – bankovní appka ho načte z obrázku.
  Nebo tlačítka **KOPÍROVAT** (účet, IBAN, VS, částka).
- Po zaplacení člověk klepne **ZAPLATIL JSEM**. U tebe v okně serveru se řádek rozsvítí
  **⏳ ČEKÁ 75 Kč** a ozve se zvuk.
- Ty platbu uvidíš v bance (VS) a v okně **KÓDY A TELEFONY** klikneš **✔ ODEMKNOUT SEGU / PS1**.
  Appka to do minuty pozná sama a napíše „SEGA ODEMČENA – díky!“.
- Zaplacené odemčení patří **kódu** – po přeinstalaci nebo na novém telefonu (UVOLNIT) zůstane.

## 3. Server v11 (NapServer_v11.zip)

- **💳 PLATBY (účet a ceny):** sem zadáš číslo účtu (stačí `123456789/0800`, server si udělá IBAN
  a zkontroluje ho), jméno a ceny. **Číslo účtu není v appce ani na GitHubu** – appka si ho bere
  ze serveru. Dokud účet nezadáš, appka napíše „Platby zatím nejsou nastavené“.
- Tabulka kódů má nové sloupce **VS Sega/PS1, Sega, PS1** (zaplaceno, čeká, postup v řádcích a hodinách).
- Tlačítka: **✔ ODEMKNOUT SEGU, ✔ ODEMKNOUT PS1, ✖ PLATBA NEPŘIŠLA** (člověk klepl ZAPLATIL, ale
  v bance nic není), **↩ ZRUŠIT ODEMČENÍ** (omyl, vrácená platba).
- Nahoře v okně: **💳 ČEKÁ NA PLATBU: n**.
- Server si zálohuje **postup** každého kódu (řádky a hodiny Segy).

## 4. Oprava z tvého testu

- Tvůj telefon měl klíč v APK, ale appka se ho při prvním spuštění vůbec nezkusila – rovnou chtěla kód.
  Stránka aktivace se zeptala appky o chvilku dřív, než ji appka zaregistrovala. **Teď počká.**
  Stejná pojistka je v N&P HRY a na obrazovce ODEMKNOUT.

## 5. Web atarihelp.eu – živé menu místo GIFů

- Na stránce stahování (page_id=1130) jsou místo GIFů **4 živé panely hlavního menu appky** (WEB, ATARI,
  SEGA, PS1). Kreslí je **přesně ten samý C++ kód jako menu v telefonu**, přeložený do WebAssembly
  (`web/menu`). Karty jdou táhnout do stran, klepnutí na kartu otevře okno pro kód (stažení appky),
  ATARIHELP.EU otevře hlavní stránku webu.
- Stránka: `web/stranka_1130/html_kod_www_stranky_B322.txt` (vložit do bloku Vlastní HTML místo staré).
- Soubor menu (2 MB) si stránka bere z GitHubu (pevná verze, kontrola SHA-256). Záloha přes FTP:
  `wp-content/uploads/nap/nap_menu_web.js`. Bez WebGL ukáže tvoje původní GIFy.

## 6. Další

- Staré cesty z JavaScriptu, které uměly přidat řádky nebo otevřít Segu bez placení
  (z dob starého Atari v JS), jsou zavřené. Řádky počítá jen C++ Atari.
- Nápověda (CZ i EN): kapitola ODEMYKÁNÍ s platbou, nové časté otázky.
- Servis ZAMKNOUT VŠE vynuluje i zálohu postupu na serveru. Zaplacené odemčení vrací jen ty v okně serveru.

## Na rovinu

- **Ověřeno na PC:** server 203 kontrol (54 nových), třídy appky proti skutečnému serveru 42, stránka
  ODEMKNOUT + aktivace v Chromiu 36 (QR kódy přečtené ZXingem = přesně text pro banku), web se živým
  menu 16, staré testy bez nové chyby kromě záměrných změn, celá appka přeložená proti Androidu 34.
- **Ověřeno v GitHubu na skutečném APK B322:** verze 365, podpis N&P (v2 + v3 s rotací), vložený klíč
  podpis nerozbije.
- **Na telefonu zatím ne** – hlavně načtení QR kódu ze skutečné bankovní appky (ČS, Fio, KB…).
- **Potvrzuješ ručně.** Když spíš, člověk čeká. Automat by šel jen s bankou, která má API (Fio).
- QR v telefonu se nedá naskenovat tím samým telefonem – proto ULOŽIT DO GALERIE. Ne každá
  bankovní appka umí QR z obrázku; pak zbývá opsat údaje (KOPÍROVAT).
