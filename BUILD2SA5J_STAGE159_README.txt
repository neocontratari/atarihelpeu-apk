AtariHelp EMU-10 BUILD2SA5J STAGE159

Proc vznikla tahle oprava:
- BUILD2SA5I omylem poslal atarihelp.eu ven do externiho Android/Nox browseru.
- To rozbilo puvodni workflow: NET HRY / SBIRKA -> AtariHelp web uvnitr appky -> klik na ZIP -> automaticky start XEX/GEN v emu.

Oprava:
1) atarihelp.eu zustava uvnitr WebView aplikace.
   - AHNet.openGames() znovu vola web.loadUrl("https://atarihelp.eu/?page_id=207").
   - atarihelp.eu uz neni v isExternalBrowserUrl whitelistu.

2) ZIP/XEX/GEN bridge zustava aktivni.
   - Po nacteni stranky atarihelp.eu se injektuje click bridge injectGameLinkBridge().
   - Klik na .zip/.xex/.atr/.com/.exe/.gen/.md/.smd/.sms/.68k/.sgd vola AHNET.runGameUrl().
   - ZIP s XEX jde do Atari/130XE cesty.
   - ZIP s GEN/MD/SMD/SMS jde do Sega cesty.

3) Ven do externiho browseru zustavaji jen cizi weby typu YouTube/Facebook.

Soucasti baliku:
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java
- app/src/main/cpp/nap_ps1_native.cpp
- app/src/main/assets/emu_ps1/index.html

Zamerne NEzmeneno:
- emu_sega/index.html neni v baliku.
- Sega native core neni v baliku.
- 130XE core/assets nejsou v baliku.

Kontrola webu:
- https://atarihelp.eu/ odpovida.
- https://atarihelp.eu/?page_id=207 odpovida.
