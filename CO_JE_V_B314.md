# Co je v B314

Tvoje zpráva: „Jo a Parťáku, šlo by dát výběr jazyka – volbu CZE a ANG? Myslím tím, že by po volbě
na ANG byla apka v angličtině? A naopak?“

## 1. Výběr jazyka – kde a jak

- **OPTIONS → JAZYK · LANGUAGE: ČEŠTINA / ENGLISH.** Přepne se hned, bez restartu, tam i zpátky.
- Dokud nic nevybereš, appka se řídí jazykem telefonu: čeština nebo slovenština = česky, cokoli
  jiného = anglicky. Tvůj telefon je česky → appka začne česky.
- Volba se pamatuje. Po odinstalování se smaže (záloha Androidu je od B313 vypnutá) a nová
  instalace zase začne podle telefonu.
- V logu: `B314 JAZYK_PRI_STARTU …`, při změně `B314 JAZYK_ZMENA -> en (OPTIONS)` a
  `B314 JAZYK_DO_KNIHOVEN …` (menu, intro, Atari).

## 2. Co se přepne

- **Hlavní menu (C++):** názvy a popisy karet (MP3 PLAYER, JUKEBOX …), tlačítka START / PLAY /
  LET'S GO / OPEN WEBSITE / OPEN GUIDE, zamčená karta LOCKED s důvodem anglicky („Type 45 more
  program lines on the Atari …“), nápověda na TV „ARROWS TO BROWSE · ENTER TO OPEN“, kniha na kartě
  HELP (GUIDE / CONTROLS).
- **OPTIONS:** všechno včetně WEB TV, kvality, TV pokusu a servisu.
- **HELP:** celý návod anglicky – vlastní anglická stránka se stejnými 13 kapitolami, názvy tlačítek
  přesně tak, jak je člověk v anglické appce uvidí, hledání anglicky.
- **Atari 130XE:** tlačítka přístroje NET / GAMES, XEX / PHONE, LOG / ERROR, lišta pod kazeťákem
  (OFF, STOPPED, RECORDING …), na šířku SPACE, úprava rozložení (IN GROUPS, DEFAULT, DONE), hlášky,
  nabídky (D-PAD AND CONTROLS, disk, kazeta, TXT), stránka LOG / ERROR a nápověda HELP.
- **Sega:** stránka, nabídky, dialogy a obrázek konzole s anglickými nápisy
  (CATALOG / SAVES / GAMES / MANUALS).
- **PS1** a **MP3 přehrávač:** všechno.
- **Hlášky Javy:** stažení her a BIOSu, aktualizace, výběr souborů, chyby sítě, zámky
  (SEGA IS LOCKED / PS1 IS LOCKED), odeslání logu.
- **Intro:** v angličtině jen anglické titulky a anglické závěrečné titulky (NOVEMBER 20, 2026 …).
  Česky zůstává přesně tak, jak jsi ho navrhl – česky a pod tím anglicky.
- **WEB TV na TV / PC:** stránka v prohlížeči anglicky (nápověda kláves, panel PICTURE, hlášky
  Ctrl+V). Když jazyk přepneš, stránku v prohlížeči obnov (F5).
- **DŽUBOX** byl anglicky už dřív – zůstává stejný v obou jazycích.
- **Co zůstává:** názvy složek v telefonu (Download/AtariHelp/Sega_hry …) – kdyby se přejmenovaly,
  appka by nenašla uložené hry a pozice. Nápisy na samotném Atari (READY, klávesy) jsou anglicky
  vždy, jako na skutečném Atari. Log appky je dál česky (ten je pro mě).

## 3. Opravy navíc

- **Atari na šířku:** popisky tlačítek se po změně jazyka nepřekreslily (obrázky tlačítek se
  pamatovaly jen podle velikosti) – teď se překreslí.
- **WEB TV:** nápověda kláves s poslední klávesou [..] by v angličtině blikala – teď se píše rovnou
  přeložená. Česky je stránka stejná jako v B313.
- **CO_JE_V_B313.md, krok 4:** chybělo, že Segu a PS1 musíš nejdřív odemknout (servis) – opraveno.

## Co prosím vyzkoušet

1. Nainstaluj B314 přes B313 (postup zůstane). Appka musí být česky jako dřív.
2. OPTIONS → JAZYK · LANGUAGE → **ENGLISH**. Menu hned anglicky (karty, tlačítka, LOCKED u zamčené
   karty). Zavři OPTIONS.
3. Projdi: ATARI 130XE (spodní tlačítka NET / GAMES …, otoč telefon na šířku – SPACE, kolečko
   ⚙ → nabídka anglicky), SEGA, PS1, MP3 PLAYER, HELP (anglický návod), OPTIONS → PLAY INTRO.
4. WEB TV: otevři (nebo obnov) adresu v prohlížeči – nápověda kláves dole anglicky.
5. OPTIONS → **ČEŠTINA** → všechno zpátky česky.
6. Pošli log z `8765/log`.

## Ověřeno na PC
