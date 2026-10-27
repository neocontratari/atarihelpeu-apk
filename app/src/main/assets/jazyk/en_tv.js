/* B314: anglicky slovnik - stranka WEB TV v prohlizeci na TV / PC (sklada ji MainActivity,
 * v anglictine do ni Java vlozi nap_jazyk.js a tenhle slovnik). Psano v ES5. */
NapJazyk.slovnik({
  'ZVUK: KLIKNI KAMKOLI DO OBRAZU': 'SOUND: CLICK ANYWHERE IN THE PICTURE',
  'prohlizec pusti zvuk az po kliknuti nebo klavese (po obnoveni stranky znovu)': 'the browser plays sound only after a click or a key press (again after reloading the page)',
  'OBRAZ': 'PICTURE',
  'jas': 'brightness',
  'kontrast': 'contrast',
  'sytost': 'saturation',
  'PUVODNI NASTAVENI': 'ORIGINAL SETTINGS',
  'OBRAZ: vyplnit obrazovku': 'PICTURE: fill the screen',
  'OBRAZ: cely, bez orezu': 'PICTURE: whole, no cropping',
  'VYLADENI OBRAZU: zapnuto': 'PICTURE TUNING: on',
  'VYLADENI OBRAZU: vypnuto': 'PICTURE TUNING: off',
  'AUDIO NENI': 'NO AUDIO',
  // napoveda ke klavesnici PC (dole)
  'ATARI - pis normalne; F1 START, F3 SELECT, F4 OPTION, F2 BREAK, F6 HELP, F8 INVERZE   (F9 = hrani)':
    'ATARI - type normally; F1 START, F3 SELECT, F4 OPTION, F2 BREAK, F6 HELP, F8 INVERSE   (F9 = game mode)',
  'ATARI HRANI - WASD/sipky smer, L = FIRE, K/mezernik skok, F1 START   (F9 = psani)':
    'ATARI GAME MODE - WASD/arrows direction, L = FIRE, K/space jump, F1 START   (F9 = typing)',
  'HRANI - WASD pohyb, K skok, L vystrel   (F9 = psani)': 'GAME MODE - WASD move, K jump, L shoot   (F9 = typing)',
  'MENU - sipky / A D listuji, Enter otevre, Home / End': 'MENU - arrows / A D browse, Enter opens, Home / End',
  'PS1 - sipky/WASD smer, K/mezernik KRIZEK, L KOLECKO, J CTVEREC, I TROJUHELNIK, Q/E L1/R1, Z/C L2/R2, Enter/P START, Backspace/O SELECT':
    'PS1 - arrows/WASD direction, K/space CROSS, L CIRCLE, J SQUARE, I TRIANGLE, Q/E L1/R1, Z/C L2/R2, Enter/P START, Backspace/O SELECT',
  'PSANI - F9 hrani, F2 BREAK, F1/F3/F4 START/SELECT/OPTION': 'TYPING - F9 game mode, F2 BREAK, F1/F3/F4 START/SELECT/OPTION',
  'SEGA - WASD pohyb, K = A, L = B, O = C, P = START': 'SEGA - WASD move, K = A, L = B, O = C, P = START',
  // B318: mys pocitace -> mys PlayStation (PS1)
  'MYS PS1: KLIKNI DO OBRAZU - KURZOR HRY OVLADA MYS POCITACE (ESC = PUSTIT)':
    'PS1 MOUSE: CLICK INTO THE PICTURE - THE COMPUTER MOUSE MOVES THE GAME CURSOR (ESC = RELEASE)',
  'MYS PS1 ZACHYCENA - POHYB A TLACITKA JDOU DO HRY (ESC = PUSTIT)': 'PS1 MOUSE CAPTURED - MOVEMENT AND BUTTONS GO TO THE GAME (ESC = RELEASE)',
  'MYS PS1 PUSTENA - KLIKNI DO OBRAZU A ZASE OVLADAS HRU': 'PS1 MOUSE RELEASED - CLICK INTO THE PICTURE TO CONTROL THE GAME AGAIN',
  'MYS PS1 JE NA TELEFONU VYPNUTA - ZAPNI JI TLACITKEM MYS': 'THE PS1 MOUSE IS OFF ON THE PHONE - TURN IT ON WITH THE MOUSE BUTTON',
  'PROHLIZEC NEDOVOLIL ZACHYTIT MYS - KLIKNI DO OBRAZU ZNOVU': 'THE BROWSER DID NOT ALLOW CAPTURING THE MOUSE - CLICK INTO THE PICTURE AGAIN',
  'TENHLE PROHLIZEC NEUMI ZACHYTIT MYS': 'THIS BROWSER CANNOT CAPTURE THE MOUSE',
  // vlozeni textu (Ctrl+V) do Atari
  'VLOZENI: MOC VELKE (NEJVYS 512 kB)': 'PASTE: TOO BIG (512 kB AT MOST)',
  'VLOZENI: PRAZDNE': 'PASTE: EMPTY',
  'VLOZENI: SPOJENI SELHALO': 'PASTE: CONNECTION FAILED',
  'VLOZENI: BEZ ODPOVEDI': 'PASTE: NO RESPONSE',
  'VLOZIT JDE JEN DO ATARI 130XE': 'PASTING WORKS ONLY INTO THE ATARI 130XE',
  // odpovedi appky na vlozeni (C++ / Java)
  'VLOZENI: TO NENI TEXT (XEX / ATR / BAS NAHRAJ V APPCE)': 'PASTE: NOT TEXT (LOAD XEX / ATR / BAS IN THE APP)',
  'VLOZENI: ATARI JE VYPNUTE': 'PASTE: THE ATARI IS OFF',
  'VLOZENI: APPKA PRAVE PISE JINY PROGRAM - POCKEJ': 'PASTE: THE APP IS TYPING ANOTHER PROGRAM - WAIT',
  'VLOZENI: ATARI 130XE NENI NA OBRAZOVCE': 'PASTE: THE ATARI 130XE IS NOT ON SCREEN',
  'VLOZENI: KNIHOVNA NENI NACTENA': 'PASTE: THE LIBRARY IS NOT LOADED',
  'VLOZENI: SPATNY POZADAVEK': 'PASTE: BAD REQUEST',
  // posledni klavesa za napovedou [..] - odpoved appky na /klavesa (C++ PcKlavesnice, PS1)
  'SKOK': 'JUMP',
  'VYSTREL': 'FIRE',
  'KRIZEK': 'CROSS',
  'CTVEREC': 'SQUARE',
  'KOLECKO': 'CIRCLE',
  'TROJUHELNIK': 'TRIANGLE',
  'NAHORU': 'UP',
  'DOLU': 'DOWN',
  'VLEVO': 'LEFT',
  'VPRAVO': 'RIGHT'
});
NapJazyk.vzory([
  [/^ZVUK (\d+) ms$/, 'SOUND $1 ms'],
  [/^POSILAM DO ATARI: (\d+) B$/, 'SENDING TO THE ATARI: $1 B'],
  [/^SOUBOR JE MOC VELKY: ([\s\S]*)$/, 'THE FILE IS TOO BIG: $1'],
  [/^SOUBOR NEJDE PRECIST: ([\s\S]*)$/, 'CANNOT READ THE FILE: $1'],
  [/^VLOZENO: (\d+) RADKU PROGRAMU \(BEZ NEW\) - (PISU AZ BUDE READY \(NEJVYS 45 S\)|PISU)( \(BEZ CISLA PRESKOCENO (\d+)\))?$/,
    function (m, n, kdy, x, bez) {
      return 'PASTED: ' + n + (n === '1' ? ' PROGRAM LINE' : ' PROGRAM LINES') + ' (NO NEW) - ' +
        (kdy === 'PISU' ? 'TYPING' : 'TYPING WHEN READY (45 S AT MOST)') +
        (x ? ' (SKIPPED ' + bez + ' WITHOUT A NUMBER)' : '');
    }],
  [/^NAPSANO: (\d+) ZNAKU( \((\d+) NA ATARI NENI\))?$/, function (m, n, x, k) {
    return 'TYPED: ' + n + (n === '1' ? ' CHARACTER' : ' CHARACTERS') + (x ? ' (' + k + ' NOT ON THE ATARI)' : ''); }],
  [/^VLOZENI: ([\s\S]*)$/, 'PASTE: $1'],
  [/^SMER:([\s\S]*)$/, 'DIRECTION:$1'],
  [/^NEZNAMA:([\s\S]*)$/, 'UNKNOWN:$1'],
  // napoveda klavesnice + posledni hlaska: "ATARI - pis normalne; ...   [VLOZENO: ...]"
  [/^([\s\S]+?)\s+\[([\s\S]*)\]$/, function (m, a, b) { return NapJazyk.tr(a) + '   [' + NapJazyk.tr(b) + ']'; }]
]);
