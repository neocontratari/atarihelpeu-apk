nap_menu_web.js = hlavni menu N&P Vision pro web (B322): PRESNE ten samy C++ jako v appce
(app/src/main/cpp/menu), prelozeny Emscriptenem 3.1.6 do WebAssembly (web/menu/vyrob.sh).
V jednom souboru je WebAssembly, obrazky karet (assets/menu), logo N&P a karty.txt.
Stranka atarihelp.eu/?page_id=1130 ho nacita pres jsDelivr (z tohoto verejneho repozitare,
pevne na commit) - nebo z https://atarihelp.eu/wp-content/uploads/nap/nap_menu_web.js (FTP).
Po zmene menu v appce: bash web/menu/vyrob.sh web/menu/vystup a commit (a nova adresa na strance).
