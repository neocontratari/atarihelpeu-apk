AtariHelp EMU-10 BUILD2SA5I STAGE158

Co je v baliku:
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativePs1CoreBridge.java
- app/src/main/cpp/nap_ps1_native.cpp
- app/src/main/assets/emu_ps1/index.html

Opravy:
1) PS1 zvuk pri odchodu z emu
   - Pridan PS1 lifecycle guard: pri odchodu z PS1 stranky, Back, onPause a onDestroy se natvrdo zastavi PS1 AudioTrack, PS1 core a zavre fd hry.
   - Boot PS1 hry ma generation guard, takze kdyz uzivatel odejde behem bootu, opozdene boot vlakno nesmi znovu spustit stary zvuk.
   - Log marker: BUILD2SA5I PS1_SESSION_STOP.

2) AtariHelp web odkazy
   - atarihelp.eu / www.atarihelp.eu / subdomeny jsou v MainActivity brane jako normalni externi web.
   - AHNet.openGames() uz neprepina WebView internim web.loadUrl, ale otevre https://atarihelp.eu/?page_id=207 pres externi browser Intent.
   - Tim se ma opravit SBIRKA / NET HRY i uvodni atarihelp.eu tlacitko, pokud tlacitka volaji tyto URL nebo AHNet.openGames().

Zamerne NEzmeneno:
- Sega asset emu_sega/index.html neni v baliku.
- Sega native core neni v baliku.
- 130XE emu core/assets nejsou v baliku.

Poznamka ke zvuku:
- Tohle resi zaseknuty zvuk po odchodu z PS1 emu.
- Kvalita zvuku v tezkych hrach typu NFS muze porad narazet na rychlost interpreter jadra. Pokud bude chrasteni i po tehle oprave, dalsi skutecny krok je vykon/dynarec, ne dalsi nafukovani fronty.
