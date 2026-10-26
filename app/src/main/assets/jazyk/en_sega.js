/* B314: anglicky slovnik - SEGA MEGA DRIVE (emu_sega/index.html). Spolecne texty Sega + PS1
 * jsou v en_konzole.js. Obrazek s anglickymi popisky dlazdic: sega_megadrive_screen_en.png. */
NapJazyk.slovnik({
  'KONZOLE': 'CONSOLE',
  'SBIRKA': 'CATALOG',
  'ULOZENE': 'SAVES',
  'HRY': 'GAMES',
  'NAVODY': 'MANUALS',
  'SEGA VYKON': 'SEGA PERFORMANCE',
  'SBIRKA = vyber vykonu, CARTRIDGE = vyber ROM': 'CATALOG = performance choice, CARTRIDGE = ROM choice',
  'MOJE HRY': 'MY GAMES',
  'Hry stažené do telefonu. Klepnutím na hru ji spustíš, koš hru smaže.': 'Games downloaded to the phone. Tap a game to start it, the bin deletes it.',
  'ULOŽENÉ POZICE': 'SAVED POSITIONS',
  'Pozice uložené ve hrách (ne export logu - to najdeš v OVLÁDÁNÍ → NÁVODY). Klepnutím na hru pozici načteš.':
    'Positions saved in games (not the log export - that is in CONTROLS → MANUALS). Tap a game to load its position.',
  'Skupina: chyť libovolné tlačítko ve skupině (akce A/B/C) a jede celá. D-pad a Start vždy zvlášť.':
    'Group: grab any button of a group (actions A/B/C) and the whole group moves. The D-pad and Start always move alone.',
  'Zatím žádná stažená hra. Vyber ji ručně nebo přes SBÍRKU.': 'No downloaded game yet. Pick one manually or from the CATALOG.',
  'Zatím žádná uložená pozice. Za hraní stiskni ULOŽIT POZICI.': 'No saved position yet. Press SAVE GAME POSITION while playing.',
  'ULOŽIT': 'SAVE',
  'NE DÍKY': 'NO THANKS',
  // monitor (obrazovka Segy)
  'SEGA C++ ONLY READY\nSBIRKA=vykon LOW/HIGH, ROM pres CARTRIDGE.\nBez ROM uz nespoustim cernou native vrstvu.':
    'SEGA C++ ONLY READY\nCATALOG=performance LOW/HIGH, ROM via CARTRIDGE.\nWithout a ROM the black native layer does not start.',
  'BUILD2RV SEGA CLEAN UI + AUDIO MASTER NOSTARVE\n\nSBIRKA:\n- vyber vykonu HIGH / LOW\n\nCARTRIDGE / CART:\n- vyber ROM v nasi appce\n- ROM jde primo do C++ bridge\n- Java/WebView wrapper je vypnuty\n- C++ CORE DOM cesta je odstranena; zadny cerny prekryv pres skin\n\nREGION:\n- C++ uz neprepisuje Europe/PAL ROM na U/NTSC\n- core respektuje header ROM\n\nMOBIL:\n- landscape cisti stare WebView vrstvy\n- portrait navrat ignoruje nulovy rect\n\nZVUK:\n- FM/PSG audio ma zeroed buffery a RR recovery safe-audit governor; S8 ma no-starve profil; LOW/HIGH nemeni barvu zvuku.':
    'BUILD2RV SEGA CLEAN UI + AUDIO MASTER NOSTARVE\n\nCATALOG:\n- performance choice HIGH / LOW (hold)\n\nCARTRIDGE / CART:\n- choose a ROM in our app\n- the ROM goes straight to the C++ bridge\n- the Java/WebView wrapper is off\n- the C++ CORE DOM path is removed; no black overlay over the skin\n\nREGION:\n- C++ no longer rewrites Europe/PAL ROMs to U/NTSC\n- the core respects the ROM header\n\nPHONE:\n- landscape clears old WebView layers\n- returning to portrait ignores a zero rect\n\nSOUND:\n- FM/PSG audio has zeroed buffers and an RR recovery safe-audit governor; the S8 has a no-starve profile; LOW/HIGH do not change the sound.',
  'BUILD2OU SEGA AUTO COMPAT\\n\\nTahle hlavni Sega vetev pouziva stabilni EU compatibility kod z BUILD2OL.\\n\\nUSA/World ROM: bez patchu.\\nEU problemove ROM: patch jen v pameti.\\nOriginal ROM soubor se nemeni.\\nZvuk: playback 48 kHz + hard reset.\\n\\nPANEL 16-BIT je ted jen info, ne prepinas vetve.':
    'BUILD2OU SEGA AUTO COMPAT\n\nThis main Sega branch uses the stable EU compatibility code from BUILD2OL.\n\nUSA/World ROM: no patch.\nProblematic EU ROMs: patched in memory only.\nThe original ROM file is not changed.\nSound: 48 kHz playback + hard reset.\n\nThe 16-BIT PANEL is just info now, it does not switch branches.'
});
NapJazyk.vzory([
  [/^Uložit „([\s\S]*)“ do telefonu\?$/, 'Save “$1” to the phone?']
]);
