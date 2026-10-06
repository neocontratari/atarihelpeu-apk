# Co je v B302

Tvoje zpráva: „Ano prosím Partaku, udělej to (klávesnici z PC do C++ Atari ve
web vieweru) – a mysli na to, že vše v C++ pokud možno.“

## Klávesnice počítače → Atari 130XE (HELP)

- **Jak:** zapni přenos na TV / PC jako v B301 a v telefonu otevři HELP
  (Atari 130XE). V prohlížeči na PC se klávesnice zapne sama (do 1 s),
  vpravo dole je nápověda. Po aktualizaci appky stránku v prohlížeči
  obnov (F5), jinak v ní zůstane stará verze.
- **Vše v C++** (nový soubor `nap_atari_pc_klavesnice.h`). Prohlížeč pošle
  jen to, co mu dal systém: která klávesa, jaký znak, Shift / Ctrl / Alt /
  AltGr. Co klávesa na Atari udělá, režim PSANÍ / HRANÍ, držené klávesy,
  tempo pro OS Atari i hlídání spojení dělá C++. Java to jen předá.

### PSANÍ (výchozí)

- Písmena, číslice a znaky, jak je máš na klávesnici, i na české:
  `" ? : + =` přes Shift, `# & @ < > [ ] \ |` přes AltGr.
- Malé písmeno = klávesa sama. Atari po startu píše VELKÁ, jako skutečné
  130XE (BASIC chce příkazy velkými). Shift = velké vždy.
- **CapsLock** = klávesa CAPS na Atari (malá písmena / zpět velká).
- ě š č ř ž ý á í é ú ů → bez háčků a čárek (Atari je nemá):
  „Příliš“ se napíše jako „PRILIS“.
- Enter = RETURN, Backspace = BACK S, Shift+Backspace = smazat řádek,
  Delete = smazat znak, Insert = vložit znak, Shift+Insert = vložit řádek,
  Home = CLEAR, Esc, Tab, **šipky = kurzor** (CONTROL + - = + *).
- Ctrl + písmeno = CONTROL (grafické znaky).
- **F1 START, F3 SELECT, F4 OPTION** (drží se, dokud držíš), **F2 BREAK**,
  F6 HELP, F8 INVERZE (klávesa s logem Atari).
- Držená klávesa: opakuje ji Atari samo, jako skutečné 130XE.

### HRANÍ (F9, zpět zase F9)

- WASD nebo šipky = joystick 1, **L = FIRE**, K nebo mezerník = skok
  (nahoru + MEZERA), F1 / F3 / F4 dál START / SELECT / OPTION.
- W / A / S / D jde zároveň jako klávesa (hry v BASICu s PEEK(764)),
  stejně jako u starého Atari emulátoru.
- Ostatní klávesy píšou (Enter, čísla v menu hry …).

## Co jsem při testech našel a opravil (jinak by se ztrácely znaky)

1. **Víc kláves naráz.** Po Wi-Fi může přijít několik kláves v jednom
   okamžiku. OS Atari stejnou klávesu hned po sobě zahodí (KEYDEL počítá jen
   s puštěnou klávesou) a dvě klávesy v jednom snímku slije do jedné. C++ je
   teď předává v tempu, které OS stihne: další klávesa nejdřív za 2 snímky,
   stejná až 5 snímků po puštění, při psaní až když OS převzal předchozí
   znak. Normálně to nepoznáš (klávesa jde do Atari v nejbližším snímku,
   do 20 ms).
   Důkaz na PC: bez tempa z „75 REM ZZZ 1001 MISSISSIPPI“ došlo
   „75 REM Z 101 MISISIPI“ a z řádků poslaných naráz skoro nic. S tempem vše.
2. **Pomalá Wi-Fi telefonu.** Prohlížeč posílal klávesy po jedné a při 25
   čekajících je zahodil. Při úsporném režimu Wi-Fi (150 ms na požadavek) a
   rychlém psaní napsala stránka z B301 v Chromiu místo
   „70 REM POMALA WIFI 0123456789 ABC…XYZ“ jen „70I MN“ a do toho se
   přimíchal další příkaz (ERROR). Teď jde, co se nahromadí, v jednom
   požadavku. Došlo všech 56 znaků.
   Pomůže to i klávesnici z PC u starého Atari emulátoru a u Segy.
3. **Šipky a 0 v prohlížeči měnily zpoždění zvuku** („ZVUK 300 ms“), takže
   psaní nuly v BASICu přenastavovalo zvuk. V Atari a Seze už ne.
4. **Zaseknutá klávesa.** Okno prohlížeče ztratí zaměření → vše se pustí.
   Spadne Wi-Fi nebo zavřeš panel s drženou klávesou → C++ po 2,5 s vše
   pustí. Jinak by Atari klávesu opakovalo donekonečna a joystick zůstal
   držený.

## Co nejde / na co pozor

- Chrome nedovolí stránce vzít si Ctrl+W, Ctrl+T, Ctrl+N (zavře / otevře
  panel) – pozor u grafických znaků přes Ctrl. Ve fullscreenu (FULL nebo
  klik do obrazu) je appka zkusí pro Atari zamknout (Chrome a Edge na PC
  to umí, Esc se pak pro odchod z fullscreenu drží). Tohle jsem ověřit
  nemohl – prohlížeč v testu fullscreen nemá.
- F5, F11 a F12 nechávám prohlížeči.
- Znaky, které Atari vůbec nemá (§ ~ { } `), se nenapíšou. Nápověda vpravo
  dole pak ukáže NEZNAMA.
- Na skutečné TV / PC s telefonem jsem to vyzkoušet nemohl. Zkoušel jsem
  na PC: skutečný prohlížeč (Chromium) se stránkou přímo z appky proti
  skutečnému jádru Atari.

## V logu

- `B302 PC KLAVESNICE: prvni klavesa …`, `rezim HRANI / PSANI`, neznámé
  klávesy, `prohlizec se 2,5 s neozval …`
- `B299 KLAVESY (PC): …` = co se z PC napsalo (řádek po RETURN).

## Ověřeno na počítači

- `test_core pc-klavesnice`: 49 kontrol, 0 chyb (řádky poslané naráz,
  dvojice a trojice stejné klávesy, česky, AltGr, CapsLock, šipky, BREAK,
  joystick, START …, hlídání spojení). Bez tempa („naivne“) 11 chyb.
- Prohlížeč Chromium + stránka přímo z `MainActivity.java` + skutečné
  jádro: 23 kontrol, 0 chyb (i pomalá Wi-Fi a spadlé spojení). Stránka
  z B301 při pomalé Wi-Fi: znaky ztracené (důkaz chyby 2).
- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: 76 kontrol, 0 chyb
  (14 nových pro B302). ThreadSanitizer: PC klávesnice ze dvou vláken
  zároveň s emulací, POWER a zastavením / spuštěním přístroje – 0 hlášení.
- Starý ATARI 130XE EMULATOR (hlavní nabídka) a SEGA: klávesnice z PC jako
  dřív, jen se už neztrácí klávesy a šipky nemění zvuk.
