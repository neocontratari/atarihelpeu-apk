/* B314: anglicky slovnik - SEGA MEGA DRIVE (emu_sega/index.html). Spolecne texty Sega + PS1
 * jsou v en_konzole.js. Obrazek s anglickymi popisky dlazdic: sega_megadrive_screen_en.png. */
NapJazyk.slovnik({
  'KONZOLE': 'CONSOLE',
  'SBIRKA': 'CATALOG',
  'ULOZENE': 'SAVES',
  'HRY': 'GAMES',
  'NAVODY': 'MANUALS',
  'SEGA VYKON': 'SEGA PERFORMANCE',
  'Podržené SBÍRKA = výkon. Při OPTIONS → VÝKON ZAŘÍZENÍ LOW hraje Sega vždy LOW.': 'Hold CATALOG = performance. With OPTIONS → DEVICE PERFORMANCE LOW the Sega always plays LOW.',
  'MOJE HRY': 'MY GAMES',
  'Hry stažené do telefonu. Klepnutím na hru ji spustíš, koš hru smaže.': 'Games downloaded to the phone. Tap a game to start it, the bin deletes it.',
  'ULOŽENÉ POZICE': 'SAVED POSITIONS',
  'Pozice uložené ve hrách. Klepnutím na hru pozici načteš. (Export logu je jinde: na šířku kolečko ⚙ → EXPORTOVAT LOG.)':
    'Positions saved in games. Tap a game to load its position. (The log export is elsewhere: in landscape the gear ⚙ → EXPORT LOG.)',
  'Skupina: chyť libovolné tlačítko ve skupině (akce A/B/C) a jede celá. D-pad a Start vždy zvlášť.':
    'Group: grab any button of a group (actions A/B/C) and the whole group moves. The D-pad and Start always move alone.',
  'Zatím žádná stažená hra. Vyber ji ručně nebo přes SBÍRKU.': 'No downloaded game yet. Pick one manually or from the CATALOG.',
  'Zatím žádná uložená pozice. Za hraní stiskni ULOŽIT POZICI.': 'No saved position yet. Press SAVE GAME POSITION while playing.',
  'ULOŽIT': 'SAVE',
  'NE DÍKY': 'NO THANKS',
  // B357: HRAT ONLINE (2 telefony pres engine N&P) - texty, ktere se meni (stav, seznam her), sklada stranka sama (NapJazyk.t)
  'HRÁT ONLINE (2 TELEFONY)': 'PLAY ONLINE (2 PHONES)',
  'HRÁT ONLINE': 'PLAY ONLINE',
  'Dva telefony, každý hraje za svého hráče – přes engine N&P. Hráč 1 hru založí (hraje se hra, která je teď v konzoli), hráč 2 ji najde v seznamu a připojí se.':
    'Two phones, each one plays for its own player – through the N&P engine. Player 1 creates the game (the game that is in the console now), player 2 finds it in the list and joins.',
  'Tvoje jméno (uvidí ho druhý hráč)': 'Your name (the other player sees it)',
  'ZALOŽIT HRU (HRÁČ 1)': 'CREATE A GAME (PLAYER 1)',
  'HRY, KTERÉ ČEKAJÍ NA HRÁČE 2': 'GAMES WAITING FOR PLAYER 2',
  'UKONČIT ONLINE HRU': 'END THE ONLINE GAME',
  // monitor (obrazovka Segy)
  // B343: obrazovka konzole bez hry (do B342 "SBIRKA=vykon" - SBÍRKA je ale seznam her N&P)
  'SEGA MEGA DRIVE – PŘIPRAVENO\nSBÍRKA = hry ze serveru N&P\nHRY = tvoje hry · CARTRIDGE = ROM z telefonu\nNÁVODY = krátký návod':
    'SEGA MEGA DRIVE – READY\nCATALOG = N&P server games\nGAMES = your games · CARTRIDGE = ROM from the phone\nMANUALS = short guide',
  // B343: NAVODY - kratky navod pro hrace
  'SEGA – KRÁTKÝ NÁVOD\nSBÍRKA = hry ze serveru N&P (podržet = výkon)\nHRY = tvoje hry · CARTRIDGE = ROM z telefonu\nULOŽENÉ = uložené pozice\nTlačítko s telefonem = hraní na šířku\nNa šířku: D-pad, A B C, START, SAVE / LOAD GAME\nKolečko ⚙ = rozložení, citlivost, levák, log\nKlávesy: WASD, K = A, L = B, O = C, P = START\nTV: ZPĚT = menu konzole\nÚplný návod: hlavní menu → karta HELP':
    'SEGA – SHORT GUIDE\nCATALOG = N&P server games (hold = performance)\nGAMES = your games · CARTRIDGE = ROM from the phone\nSAVES = saved positions\nButton with a phone = play in landscape\nLandscape: D-pad, A B C, START, SAVE / LOAD GAME\nGear ⚙ = layout, sensitivity, left-handed, log\nKeys: WASD, K = A, L = B, O = C, P = START\nTV: BACK = console menu\nFull guide: main menu → the HELP card',
  'BUILD2OU SEGA AUTO COMPAT\\n\\nTahle hlavni Sega vetev pouziva stabilni EU compatibility kod z BUILD2OL.\\n\\nUSA/World ROM: bez patchu.\\nEU problemove ROM: patch jen v pameti.\\nOriginal ROM soubor se nemeni.\\nZvuk: playback 48 kHz + hard reset.\\n\\nPANEL 16-BIT je ted jen info, ne prepinas vetve.':
    'BUILD2OU SEGA AUTO COMPAT\n\nThis main Sega branch uses the stable EU compatibility code from BUILD2OL.\n\nUSA/World ROM: no patch.\nProblematic EU ROMs: patched in memory only.\nThe original ROM file is not changed.\nSound: 48 kHz playback + hard reset.\n\nThe 16-BIT PANEL is just info now, it does not switch branches.'
});
NapJazyk.vzory([
  [/^Uložit „([\s\S]*)“ do telefonu\?$/, 'Save “$1” to the phone?']
]);
