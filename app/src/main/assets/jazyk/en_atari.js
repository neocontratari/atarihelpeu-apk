/* B314: anglicky slovnik - zadni strana Atari 130XE (emu_atari_cpp/index.html: log a test jadra).
 * Cesky -> anglicky, cela veta tak, jak ji stranka vypisuje (assets/spolecne/nap_jazyk.js). */
NapJazyk.slovnik({
  'ATARI 130XE — log a testy': 'ATARI 130XE — log and tests',
  'ATARI 130XE v C++': 'ATARI 130XE in C++',
  'Zadni strana pristroje: log a testy. Jedine Atari v appce (karta ATARI 130XE v hlavnim menu); stare Atari v JavaScriptu je od B310 smazane.':
    'The back of the machine: log and tests. The only Atari in the app (the ATARI 130XE card in the main menu); the old JavaScript Atari was removed in B310.',
  'ZPET NA ATARI 130XE': 'BACK TO THE ATARI 130XE',
  'Co teď testuješ? (např. CSAVE po RETURN)': 'What are you testing now? (e.g. CSAVE after RETURN)',
  'ZAPSAT': 'WRITE',
  'CO TESTOVAT': 'WHAT TO TEST',
  'TEST JADRA': 'CORE TEST',
  'Test nespusten': 'Test not started',
  'SPUSTIT TEST JADRA': 'RUN THE CORE TEST',
  'Procesor 6502': '6502 processor',
  'Pamet a banky': 'Memory and banks',
  'Rychlost': 'Speed',
  'Architektura': 'Architecture',
  'CPU: instrukci': 'CPU: instructions',
  'CPU: soucet telefon': 'CPU: checksum phone',
  'CPU: soucet pocitac': 'CPU: checksum computer',
  'Pamet: cteni': 'Memory: reads',
  'Pamet: soucet telefon': 'Memory: checksum phone',
  'Pamet: soucet pocitac': 'Memory: checksum computer',
  'char je se znamenkem': 'char is signed',
  'Soubor teto relace': 'This session’s file',
  'Velikost': 'Size',
  'Soubor z minula (po padu)': 'Previous file (after a crash)',
  'Naposledy ulozeno do': 'Last saved to',
  '(zatim nic)': '(nothing yet)',
  '(nevytvoren)': '(not created)',
  '(zadny)': '(none)',
  'ULOZIT LOG A ODESLAT': 'SAVE AND SEND THE LOG',
  'OBNOVIT VYPIS NIZE': 'REFRESH THE LISTING BELOW',
  'Jak mi log poslat': 'How to send me the log',
  'Zpet do nabidky': 'Back to the menu',
  'Nativni knihovna napatari se nenacetla': 'The native library napatari did not load',
  'POCITAM...': 'COMPUTING...',
  'Test bezi, chvili to potrva': 'The test is running, it takes a moment',
  'ZKUSIT ZNOVU': 'TRY AGAIN',
  'SPUSTIT ZNOVU': 'RUN AGAIN',
  'SEDI': 'MATCHES',
  'NESEDI': 'DOES NOT MATCH',
  'ano': 'yes',
  'ne': 'no',
  'OBOJI SEDI — jadro se na telefonu chova stejne jako na pocitaci': 'BOTH MATCH — the core behaves the same on the phone as on the computer',
  'NESEDI — jadro se na ARM chova jinak': 'DOES NOT MATCH — the core behaves differently on ARM',
  'Zatim nic odklepnuto.': 'Nothing marked yet.'
});
NapJazyk.vzory([
  [/^ULOZENO \((\d+) znaku\) — vyber, cim poslat$/, 'SAVED ($1 characters) — choose how to send it'],
  [/^NEPOVEDLO SE: ([\s\S]*)$/, 'FAILED: $1'],
  [/^Nepodarilo se spustit: ([\s\S]*)$/, 'Could not start: $1'],
  [/^Jadro hlasi chybu: ([\s\S]*)$/, 'The core reports an error: $1'],
  [/^nelze zjistit: ([\s\S]*)$/, 'cannot find out: $1'],
  [/^nelze precist: ([\s\S]*)$/, 'cannot read: $1'],
  [/^([\d.]+) MIPS {2}\(([\d.]+)x Atari\)$/, '$1 MIPS  ($2x Atari)'],
  [/^(\d+) {2}za ([\d.]+) s$/, '$1  in $2 s'],
  [/^V poradku: (\d+) {3}Spatne: (\d+) {3}Zbyva: (\d+)([\s\S]*)$/, function (m, a, b, c, d) {
    return 'OK: ' + a + '   Wrong: ' + b + '   Left: ' + c +
      d.replace('   >>> POSLI MI LOG, neco nesedi.', '   >>> SEND ME THE LOG, something is off.')
       .replace('   >>> Vse odklepnuto, posli mi log.', '   >>> All marked, send me the log.');
  }]
]);
// seznam kroku "CO TESTOVAT" (od B314 prazdny - kroky jsou ve zprave; kdyby se vratil)
NapJazyk.slovnik({
  'U kazdeho kroku klepni V PORADKU nebo SPATNE. Zapise se to do logu, takze mi pak staci poslat log a nic nemusis psat.':
    'For each step tap OK or WRONG. It is written to the log, so you only need to send me the log and do not have to write anything.',
  'V PORADKU': 'OK',
  'SPATNE': 'WRONG'
});
NapJazyk.vzory([
  [/^SPRAVNE: ([\s\S]*)$/, 'RIGHT: $1'],
  [/^SPATNE: ([\s\S]*)$/, 'WRONG: $1']
]);
