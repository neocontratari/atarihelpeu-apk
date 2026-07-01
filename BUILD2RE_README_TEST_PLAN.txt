AtariHelp.eu EMU-10 - BUILD2RE
SEGA_NATIVE_CPP_ONLY_CLEAN_UI_LANDSCAPE_PERF_STAGE121

NAVAZUJE NA:
- BUILD2RD: Nox OK, navrat z Atari 130XE OK, Samurai OK.
- S8 problem zustal: cerny obdelnik na uvodni Sega obrazovce, landscape orez/HUD, postupne zpomalovani videa+zvuku po delsim hrani / vice ROM.

CO BUILD2RE MENI:
1) SEGA UI CLEAN
- Odstranen fake cerny obdelnik cppCoreArtCover z HTML/CSS/DOM.
- C++ CORE DOM tlacitko zustava odstranene.
- Poznamka: pokud je napis C++ CORE primo vypalen v PNG skinu, musi se opravit zdrojovy PNG skin; BUILD2RE uz nedela cernou zaplatu pres obraz.

2) LANDSCAPE BEZ OREZU HUD/ZIVOTU
- Android TextureView kresli Mega Drive frame 320x224 jako aspect-fit do landscape view.
- Obraz hry se nesmi orezat; score/rings/zivoty musi byt videt cele.
- Muze vzniknout cerny okraj po stranach/nahore/dole, ale nesmi chybet kus herniho obrazu.

3) S8/A12 PERFORMANCE GUARD
- Render thread na Android <= 9 nejede natvrdo 60 FPS, ale umi spadnout na cca 30 FPS/25 FPS pri vysokem render cost.
- Core/audio zustavaji v real timing rezimu; dropuje se pouze zobrazovani, aby mobil mene hral a nezacal thermal throttle.
- Audio QT/QP profil zustava zachovan.

4) AUDIO CALLBACK ALLOCATION CLEANUP
- C++ FM/PSG/PCM/CDDA callbacky uz nealokuje std::vector pri kazdem callbacku.
- Pouziva thread_local buffer, aby se snizil tlak na CPU/heap a zahrev po delsim hrani.

5) LEPSI LOG PRO S8
- Loguje NATIVE_RENDER_PERF_RE: frames za 5 s, pocet slow frame, render cost, Android verze, landscapeFull.
- Loguje NATIVE_TEXTURE_FRAME_RE s gameDst, aby bylo jasne, jestli obraz je aspect-fit nebo orezany.

TEST PLAN:
A) Workflow
1. Aplikuj overlay pres BUILD2RD/BUILD2RC aktualni vetvi.
2. Spust GitHub Actions.
3. Ocekavani: vznikne jedna APK bez krizku.

B) Nox kontrola
1. Sega -> SBIRKA -> Sonic.
2. Atari 130XE -> zpet Sega -> Sonic/Aladdin.
3. Ocekavani: bez cerne obrazovky se zvukem, zadny maly obraz vlevo nahore.

C) S8 landscape layout
1. Sonic -> otocit na sirku.
2. Ocekavani: score/rings/zivoty cele videt, zadny orez spodku.
3. A/B/C dole vpravo, DPAD dole vlevo, citlivost zachovana.

D) S8 dlouhy beh
1. Sonic 10-15 minut bez vymeny ROM.
2. Pokud se zacne zpomalovat, uloz LOG hned v momente zpomaleni.
3. V logu hledat NATIVE_RENDER_PERF_RE, audio underruns, frameCounter/displayFrameCounter.

E) S8 multi-ROM
1. Sonic 30 s -> Aladdin 30 s -> Samurai 30 s -> Sonic 30 s.
2. Ocekavani: rychlost se nesmi zhorsovat periodickou radou.
3. Pokud se zhorsuje, poslat normalni SEGA LOG + C++ INPLACE LOG.
