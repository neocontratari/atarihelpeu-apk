/* B314: anglicky slovnik - spolecne texty konzoli Sega a PS1 (panel D-PAD A OVLADANI,
 * profi nastaveni ovladani, uprava rozlozeni, moje hry, mazani hry). Cesky -> anglicky. */
NapJazyk.slovnik({
  // panel D-PAD A OVLADANI
  'D-PAD A OVLÁDÁNÍ': 'D-PAD AND CONTROLS',
  'UPRAVIT ROZLOŽENÍ TLAČÍTEK': 'EDIT BUTTON LAYOUT',
  'OVLÁDÁNÍ: CITLIVOST A VZHLED': 'CONTROLS: SENSITIVITY AND LOOK',
  'PROHODIT D-PAD A AKCE (LEVÁK)': 'SWAP D-PAD AND ACTIONS (LEFT-HANDED)',
  'ULOŽIT POZICI VE HŘE': 'SAVE GAME POSITION',
  'NAČÍST ULOŽENOU POZICI': 'LOAD SAVED POSITION',
  'EXPORTOVAT LOG (DIAGNOSTIKA)': 'EXPORT LOG (DIAGNOSTICS)',
  'ULOŽIT LOG A ODESLAT': 'SAVE AND SEND THE LOG',
  'ZAVŘÍT': 'CLOSE',
  'ZAVRIT': 'CLOSE',
  // uprava rozlozeni
  'Přetáhni tlačítko, kam chceš.': 'Drag a button wherever you like.',
  'Přetáhni tlačítko, kam chceš. Jde se vrátit i na výchozí.': 'Drag a button wherever you like. You can also go back to the default.',
  'PO SKUPINÁCH': 'IN GROUPS',
  'PO JEDNOM': 'ONE BY ONE',
  'VÝCHOZÍ': 'DEFAULT',
  'HOTOVO': 'DONE',
  // profi nastaveni ovladani
  'OVLÁDÁNÍ — profi nastavení': 'CONTROLS — pro settings',
  'Citlivost D-padu': 'D-pad sensitivity',
  '— jak daleko od středu musí prst dojet, než se směr sepne.': '— how far from the center your finger has to move before a direction switches on.',
  '— jak daleko od středu musí prst dojet, než se směr sepne. Nižší = citlivější (sepne dřív), vyšší = musíš jet víc do kraje.':
    '— how far from the center your finger has to move before a direction switches on. Lower = more sensitive (switches sooner), higher = you have to move further to the edge.',
  'Průhlednost tlačítek': 'Button transparency',
  '— jak moc vidíš skrz ovladač na hru pod ním.': '— how much of the game you see through the controller.',
  'Velikost tlačítek': 'Button size',
  '— A/B/C, Start.': '— A/B/C, Start.',
  '— Triangle/Circle/Cross/Square, ramena, Select/Start.': '— Triangle/Circle/Cross/Square, shoulders, Select/Start.',
  'Velikost D-padu': 'D-pad size',
  '— jen kulatý ovladač, samostatně.': '— just the round pad, on its own.',
  'Vibrace při stisku tlačítka': 'Vibrate when a button is pressed',
  'Průhledný ovladač (rychlá volba - někdo chce vidět skrz, někdo ne)': 'See-through controller (quick choice - some want to see through it, some don’t)',
  'VÝCHOZÍ NASTAVENÍ OVLÁDÁNÍ': 'DEFAULT CONTROL SETTINGS',
  'Ovládání vráceno na výchozí': 'Controls reset to default',
  'D-pad a akční tlačítka prohozeny (levák) - uloženo.': 'D-pad and action buttons swapped (left-handed) - saved.',
  // moje hry / mazani
  'OBNOVIT SEZNAM': 'REFRESH THE LIST',
  'VYBRAT SOUBOR RUČNĚ V TELEFONU': 'PICK A FILE ON THE PHONE MANUALLY',
  'SMAZAT HRU': 'DELETE GAME',
  'Co přesně chceš smazat?': 'What exactly do you want to delete?',
  'JEN HRU (uložená pozice zůstane)': 'JUST THE GAME (the saved position stays)',
  'HRU I ULOŽENOU POZICI': 'THE GAME AND THE SAVED POSITION',
  'ZRUŠIT': 'CANCEL',
  'Načítám…': 'Loading…',
  'Spustit': 'Start',
  'Smazat': 'Delete',
  'Načíst': 'Load',
  'uložená pozice': 'saved position',
  'uložená hra': 'saved game'
});
NapJazyk.vzory([
  [/^Citlivost: (\d+) %$/, 'Sensitivity: $1 %'],
  [/^Průhlednost: (\d+) %$/, 'Transparency: $1 %'],
  [/^Velikost: (\d+) %$/, 'Size: $1 %'],
  [/^Velikost D-padu: (\d+) %$/, 'D-pad size: $1 %']
]);
