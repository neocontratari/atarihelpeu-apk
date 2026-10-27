# Co je v B319

Tvoje zpráva: „Je to mnohem mnohem lepší – ale mám pocit, že došlo ke zpomalení při přenosu LAN … Jinak super,
na PC se to ovládá pěkně s tou myší.“

## 1. Zpomalení přenosu LAN – nebyl to jen pocit

- **Z tvého logu B318:**
  - telefon hrál plných 60 snímků za sekundu;
  - na PC ale chodilo jen **48–57 snímků za sekundu** (v B317 57–59);
  - zvuk byl za obrazem asi **0,2 s** (v B317 asi 0,15 s);
  - appka kvůli tomu i snižovala kvalitu obrazu.
- **Proč:**
  - když posouváš mapu (s myší z PC teď hodně), Dune překreslí celý obraz 20× za sekundu;
  - to bylo až **6 Mbit/s** (v B317 nejvýš 2,3) a Wi-Fi to nestíhala;
  - navíc myš z PC posílala asi 25 zpráv za sekundu a každá otevírala nové spojení.
- Kód přenosu se od B313 nezměnil. Zpomalení způsobilo víc dat, ne pomalejší appka.

## 2. Co B319 dělá

- **Posun mapy:** když se obraz jen posunul, appka pošle „posuň obraz o tolik“. K tomu pošle jen to,
  co je opravdu nové: pruh na kraji, jednotky a kurzor. Prohlížeč si zbytek posune sám.
- **Obraz zvětšený 2×:** u 2D her (Dune) je obraz s nastavením „2x (Enhanced Resolution)“ jen každý bod
  zvětšený na čtyři. Teď jde v původním rozlišení a prohlížeč ho zvětší přesně stejně – vypadá stejně,
  dat je méně.
- **Myš z PC:**
  - posílá jedním trvalým spojením (dřív každá zpráva nové spojení);
  - nejvýš asi 30 zpráv za sekundu (hra kreslí 20× za sekundu), tlačítka jdou hned.
- Prohlížeč otevřený ještě před aktualizací dostane přenos jako dřív. Nový přenos má po obnovení
  stránky (F5).
- Sega, Atari a hlavní menu jedou na TV jako dřív.

## 3. Poctivě

- **Dune 2000 nemám.** Že je obraz v 2× přesně zdvojený a že se mapa posouvá „čistě“, předpokládám podle
  toho, jak jádro PS1 kreslí 2D hry. Tvůj příští log to ukáže:
  - řádek `B306 TV PROUD PS1 … B319: posun obrazu … x … casti v polovicnim rozliseni … z …`;
  - řádek `B306 PROHLIZEC PS1: … B319: kopii (posun obrazu) …/s`.
- Kdyby to u Dune nesedělo, přenos pojede jako v B318 – nic se nezhorší. Pak to doladím podle logu.
- Slabá Wi-Fi daleko od routeru zůstává. B319 posílá méně dat, takže by mělo stačit i slabší spojení.

## Co prosím vyzkoušet

1. Nainstaluj B319 (postup zůstává).
2. Zapni WEB TV a na PC obnov stránku (F5).
3. Dune 2000 s myší z PC: posouvej mapu (kurzor k okraji), vybírej rámečkem, hraj jako minule.
4. Kolečko → **ULOŽIT LOG A ODESLAT** → pošli log.

## Ověřeno na PC

- **Umělá 2D strategie jako Dune 2000** (obraz 368×240 a 2× = 736×480, posun mapy všemi směry,
  jednotky, kurzor; skutečný kodér přenosu z appky):
  - posun mapy: **4× méně dat** (67,9 kB → 17,2 kB na snímek; při 20 snímcích za sekundu
    ~11 Mbit/s → ~2,8 Mbit/s);
  - mapa stojí (jednotky, kurzor): 31,2 kB → 5,7 kB na snímek;
  - obraz věrný jako dřív, spíš lepší (PSNR 30,8 dB, dřív 30,4 dB);
  - posun se najde i tehdy, když prohlížeč nějaký snímek vynechá;
  - po zastavení se obraz zjemní jako dřív;
  - telefon to kóduje rychleji (1,7 ms místo 2,7 ms na snímek).
- **3D hra** (otáčení, zoom): nic se nemění – žádná kopie, stejná data, stejná rychlost.
- **Skutečná stránka WEB TV v Chromiu** se skutečným přenosem: obraz na PC = snímek hry v 11 kontrolních
  bodech uprostřed posunu; dat 2,17 MB místo 9,09 MB; zvětšení 2× přesné (žádné rozmazání);
  stará stránka B318 dostane přenos jako dřív a funguje.
- **Myš z PC:** 180 zpráv jen 2 spojeními (B318: 180 spojení); nejvýš ~30 zpráv za sekundu, součet pohybu
  přesný, tlačítko hned. Vše z B318 (zachycení, Esc, tlačítka) beze změny.
- **Skutečný kód serveru z appky** (spojení, které zůstává) vyzkoušený v Javě s Chromiem: spojení se po 5 s
  ticha samo zavře, starší prohlížeče a ostatní požadavky jako dřív.
- **Regrese – 27 OK, 0 CHYBA:**
  - přenos Sega / PS1 / menu ze starších verzí s novým kodérem – bez nových voleb přesně jako dřív;
  - PS1 jako dřív (deník, klávesnice PC, stránka PS1), myš B318 i se skutečným jádrem PS1 (36/0);
  - Java ↔ C++ sedí (29 propojení), celá appka přeložená proti Androidu bez chyby;
  - stránka WEB TV ve starších testech: zvuk 20/0, přepnutí proudu 3/0, klávesnice PC do Atari 23/0,
    Atari na TV 23/0, Sega / PS1 / demo 37/0, čeština i angličtina jako dřív; skutečné hlavní menu 15/0.
