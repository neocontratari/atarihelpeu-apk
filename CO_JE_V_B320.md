# Co je v B320

Tvoje zpráva: „Proste to riskni … je to pro nás výzva – Retroid Pocket naslepo.“
Uživatel z fóra: „Jestli to půjde na Retroid Pocket 6 s tím, že by se na obrazovce po připojení stáhnul
náš D-pad a najelo to na originál tlačítka.“

## 1. Herní ovladač – Retroid Pocket 6 naslepo (i Bluetooth / USB gamepad)

- **Fyzická tlačítka fungují** v PS1, na Seze, v Atari 130XE i v hlavním menu.
- **Dotykový D-pad a tlačítka se schovají**, jakmile hraje ovladač (první stisk nebo pohyb páčkou).
  - Ťukneš na místo, kde byla → jsou zpátky (a tlačítko rovnou funguje). Další stisk na ovladači je zase schová.
  - Ovladač odpojíš → dotyková tlačítka jsou zpátky.
  - Ozubené kolečko, tlačítko MYŠ a úprava rozložení tlačítek zůstávají vždy.
  - Platí na šířku – PS1, Sega i Atari 130XE. Na výšku (ovládání pod obrazem) se nic neschovává.
- **OPTIONS → HERNÍ OVLADAČ:** AUTOMATICKY (jak je popsáno výš), VŽDY DOTYK (nic se neschovává),
  SKRÝT DOTYK (dotyková tlačítka ve hrách nejsou vůbec). Pod tím je jméno ovladače.
- **Tlačítka podle místa na ovladači** (jak je hlásí Android):
  - **PS1:** spodní = KŘÍŽEK, pravé = KOLEČKO, levé = ČTVEREC, horní = TROJÚHELNÍK, L1 R1 L2 R2,
    stisk páček L3 / R3, START, SELECT.
  - **Sega:** levé = A, spodní = B, pravé = C (A B C v řadě zleva), START.
  - **Atari:** spodní / pravé / L1 / R1 = FIRE, levé = MEZERA, horní = RETURN, START, SELECT,
    L2 / R2 = OPTION.
  - **Hlavní menu:** křížek nebo L1 / R1 listují, A nebo START spustí, SELECT nebo Y = OPTIONS.
    B v menu nedělá nic (jinak by Android appku poslal do pozadí).
  - Tlačítko ZPĚT funguje jako dřív.
- **Páčky:** levá páčka = směr (s rezervou, aby necukala), analogové spouště L2 / R2 fungují.
  V Dune 2000 s myší PlayStation **pravá páčka = myš** – rychlost podle vychýlení, naplno asi jedna šířka
  obrazu za sekundu (citlivost myši platí i tady).
- **Opravená chyba z dřívějška:** u PS1 bylo na ovladači B = čtverec, X = SELECT, Y = START,
  SELECT = kolečko, START = trojúhelník. Nikdo si nestěžoval, protože s ovladačem nikdo nehrál.
  Teď je to podle místa jako na ovladači PlayStation. Opravené jsou i klávesy na Bluetooth klávesnici
  (jako klávesnice PC na TV: šipky / WASD, K křížek, L kolečko, J čtverec, I trojúhelník, Enter START).
- **Poctivě:** Retroid ani jiný ovladač nemám. Vše jsem ověřil na PC se skutečným kódem appky (jádra PS1,
  Sega a Atari, stránky, Java). Na skutečném zařízení to nikdo nezkoušel.
  - Kdyby na Retroidu bylo křížek a kolečko prohozené, je to nastavením rozložení tlačítek v Retroidu
    (Xbox / Nintendo). Appka bere tlačítka tak, jak je hlásí Android.
  - Appka zapíše do logu všechno o ovladači: jméno, tlačítka, páčky, první stisky – řádky **„B320 OVLADAC“**.
    Z jednoho logu z fóra uvidím, co Retroid opravdu posílá.

## 2. Dune 2000 na TV / PC – příčina nalezena a opravena

- **Z tvého logu B319:** posun obrazu zkoušen 322×, nalezen 0×; v polovičním rozlišení jen 304 částí z 1137;
  „dithering ZAPNUTO“.
- **Příčina** (ověřená na skutečném vykreslovači PS1 z appky, ne odhadem): PS1 kreslí mapu s ditheringem.
  Jeho mřížka je pevně na obrazovce. Po posunu mapy o lichý počet bodů má stejný kus mapy body o odstín
  jinak. B319 chtěla přesnou shodu, tak kopii nenašla nikdy.
- **B320:** hledá posun s tolerancí jednoho odstínu. Rozdíl, který oko nevidí, se dorovná, jakmile obraz
  stojí. Hry bez ditheringu jedou přesně jako v B319.
- **Na skutečném vykreslovači (mapa s ditheringem jako Dune):**
  - posun mapy nalezen ve **108 ze 108** snímků posunu (B319 při posunu o lichý počet bodů 0 z 55);
  - dat **17 %** proti dřívějšku (B319 78 %);
  - obraz v prohlížeči stejně věrný jako dřív.
- Poloviční rozlišení u mapy s ditheringem nejde (mapa ve 2× není přesně zdvojená). Ušetří to ale kopie.

## 3. Stránka WEB TV se po aktualizaci appky obnoví sama

- Stránka zná verzi appky. Když se appka aktualizuje, stránka se sama obnoví (nejvýš jednou za 30 s).
- **Jen tentokrát ještě F5:** stránka z B319 to ještě neumí. Od B320 už F5 nebude potřeba.

## 4. Nastavení (vzhled)

- Nový vzhled nastavení jsem nedělal – čekám na tvoje potvrzení.

## Co prosím vyzkoušet

1. Nainstaluj B320 (postup zůstává).
2. Na PC jednou obnov stránku WEB TV (F5).
3. Dune 2000 na TV: posouvej mapu myší z PC – mělo by to jet plynuleji.
4. Pokud máš po ruce jakýkoli Bluetooth / USB ovladač, zkus PS1, Segu, Atari a menu. Na fóru prosím
   o log z Retroidu.
5. Kolečko → **ULOŽIT LOG A ODESLAT** → pošli log.

## Ověřeno na PC

- **Skutečný vykreslovač PS1 z appky** (2× obraz, dithering): 15/0 – výsledky výš.
- **Kodek s ditheringem** (umělá Dune 2× i 1×, 3D hra, hra bez ditheringu = B319): 12/0.
- **Ovladač – logika** (tlačítka podle místa, páčky, spouště, myš páčkou): 20/0.
- **Ovladač v PS1 se skutečným jádrem PCSX:** 16 tlačítek, víc najednou, dotyk + ovladač současně,
  odpojení uprostřed stisku, myš pravou i levou páčkou, deník: 14/0; ThreadSanitizer: 0 hlášení.
- **Ovladač v Atari** (skutečné jádro): joystick, FIRE, MEZERA, RETURN, konzole, schování D-padu
  na šířku: 14/0.
- **Ovladač na Seze** (skutečné jádro clownmdemu): 5/0.
- **Java** (rozpoznání ovladače, Xbox / DualShock / jiné páčky, schování, režimy, propojení s C++): 20/0.
- **Stránky PS1, Sega a OPTIONS v Chromiu** (česky i anglicky, bez ovladače přesně jako dřív): 13/0.
- **Stránka WEB TV se obnoví sama:** 6/0.
- **Regrese – 33 OK, 0 CHYBA:**
  - všechny testy B319 na kódu B320: přenos bez ditheringu přesně jako B319, stránka WEB TV, myš z PC,
    jedno spojení;
  - přenos Sega / PS1 / menu ze starších verzí, PS1 jako dřív (deník, klávesnice PC, stránka PS1),
    myš B318 i se skutečným jádrem PS1 (36/0);
  - celá appka přeložená proti Androidu bez chyby; Java ↔ C++ sedí ve všech 3 mostech (102 propojení);
  - stránka WEB TV ve starších testech: zvuk 20/0, přepnutí proudu 3/0, klávesnice PC do Atari 23/0,
    Atari na TV 23/0, Sega / PS1 / demo 37/0, čeština i angličtina jako dřív; skutečné hlavní menu 15/0;
  - stránka WEB TV se od B319 liší jen 3 změnami B320 (verze appky).
