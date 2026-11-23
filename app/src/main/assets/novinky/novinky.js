/* CO JE NOVEHO - zasada Reneho (23. 11. 2026): "v kazde aktualizaci musi byt videt co je noveho - to je zasada".
   KAZDY build sem pridava svuj zaznam NAHORU: "verze" = versionCode z app/build.gradle, "build" = Bxxx z versionName.
   Hlida to CI (Build APK, krok "Co je noveho") skriptem test_overeni/b359/kontrola_novinek.py - bez zaznamu se APK
   nesestavi. Appka po aktualizaci jednou ukaze zaznamy, ktere telefon jeste nevidel (MainActivity.napNovinky,
   NapNovinky.java), Napoveda je ma nahore v sekci CO JE NOVEHO. Pole za NAP_NOVINKY = musi byt cisty JSON (Java ho
   cte pres NapJson) - zadne komentare ani carky navic. Texty pro lidi: kratce, co se zmenilo a kde to najdou. */
window.NAP_NOVINKY = [
  {"verze": 402, "build": "B359", "datum": "23. 11. 2026",
   "cs": ["Co je nového: po každé aktualizaci appka jednou ukáže, co se změnilo. Kdykoli později to najdeš v Nápovědě úplně nahoře (CO JE NOVÉHO)."],
   "en": ["What's new: after every update the app shows once what has changed. Any time later you find it at the very top of Help (WHAT'S NEW)."]},
  {"verze": 401, "build": "B358", "datum": "23. 11. 2026",
   "cs": ["Sega online je jednodušší: velké tlačítko 🌐 ONLINE – na výšku pod ovladačem, na šířku dole vedle LOAD GAME.",
          "Okno ONLINE ukáže, co dělat. Když založíš hru, uvidíš velké číslo hry a návod pro druhý telefon.",
          "Seznam her se obnovuje sám a u každé hry píše, co je potřeba: máš ji / stáhne se ze serveru / spustí se z HRY / nemáš ji.",
          "Hráč 2 nemusí hru předem spouštět, když ji má v HRY. Bez hry tě appka pošle VYBRAT HRU a pak tě vrátí zpátky.",
          "Všechno kolem online se zapisuje do logu – každý pokus má číslo a u konce je důvod."],
   "en": ["Sega online is simpler: a big 🌐 ONLINE button – in portrait under the controller, in landscape at the bottom next to LOAD GAME.",
          "The ONLINE window shows what to do. When you create a game you see a big game number and instructions for the other phone.",
          "The game list refreshes by itself and says for each game what is needed: you have it / it downloads from the server / it starts from GAMES / you do not have it.",
          "Player 2 does not have to start the game first when it is in GAMES. Without a game the app sends you to PICK A GAME and brings you back.",
          "Everything around online is written into the log – every attempt has a number and the reason it ended."]},
  {"verze": 400, "build": "B357", "datum": "20. 11. 2026",
   "cs": ["Novinka: Sega online – dva telefony přes engine N&P, každý hraje za svého hráče. Hra běží na obou telefonech přesně stejně."],
   "en": ["New: Sega online – two phones through the N&P engine, each plays for its own player. The game runs exactly the same on both phones."]},
  {"verze": 399, "build": "B356", "datum": "19. 11. 2026",
   "cs": ["Android 10 a novější (třeba Xiaomi): když složka Download/AtariHelp nejde zapsat, appka uloží hry, BIOS i paměťovou kartu do své složky.",
          "Stahování PS1 her se po zaseknutí samo naváže a hra s .cue se stáhne celá ze serveru N&P.",
          "PS1: SAVE GAME bez hry (jen BIOS) už nepřepíše pozici hry a chvilku po otočení obrazovky se klik nebere.",
          "Když appka spadne, důvod se zapíše do logu – i z minulého běhu."],
   "en": ["Android 10 and newer (e.g. Xiaomi): when the Download/AtariHelp folder cannot be written, the app keeps games, BIOS and the memory card in its own folder.",
          "PS1 game downloads resume by themselves after a stall, and a game with a .cue downloads completely from the N&P server.",
          "PS1: SAVE GAME without a game (BIOS only) no longer overwrites the game's position, and a tap right after rotating the screen is ignored.",
          "When the app crashes, the reason goes into the log – even from the previous run."]}
];
