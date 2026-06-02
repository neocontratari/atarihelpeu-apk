EMU-09 FIX77 RELEASE + 240 VIEWPORT + COBRA UI

Co je nove proti FIX76:

- Ikona aplikace je znovu natvrdo N&P pecet z pecet.png.
- N&P logo je v assets jako np_logo.png, EMU-09 obrazovka ho ukazuje primo nahore a WebView cache se pri startu cisti.
- GitHub Actions vyrabi release APK: assembleRelease a artifact HOTOVE_RELEASE_APK_ATARIHELP_STAHNI_ME / app-release.apk.
- Manifest nema INTERNET opravneni, ma app_icon i roundIcon nastaveny na N&P pecet a allowBackup=false.
- Release APK je podepsane stabilnim N&P podpisem app/atarihelp-release.p12.
- Poznamka k Play Protect: sideload mimo Google Play muze porad zobrazit varovani "Presto nainstalovat". Kod to umi jen zlepsit release buildem, ne garantovane vypnout bez duveryhodne distribuce.
- Obecny ANTIC renderer ma 320x240 viewport. To je dulezite pro Super Cobru, kde spodni FUEL/HUD panel nebyl ve 192 radcich vubec videt.
- Mobilni ovladani je prestavene na levy palcovy kriz: nahoru, dolu, vlevo, vpravo.
- FIRE je velke tlacitko vpravo pro pravou ruku.
- Atari klavesnice je presunuta pod levy kriz.
- START uprostred krize je pryc. TAP START je nad FIRE.
- Smerova tlacitka a FIRE maji pointer capture, aby pri malem pohybu palce hned nevypinaly.
- PiTT-KiTT textove obrazovky maji cisty background a citelny fallback foreground.
- Super Cobra ma uzsi missile kolizi: playfield zasah se bere hlavne ze spicky rakety, ne z velkeho nafouknuteho obdelniku.
- Super Cobra FIRE z velkeho mobilniho tlacitka je kratky puls, aby drzeni tlacitka nevypadalo jako nesmyslna strelba.
- Super Cobra uz nedeferuje HITCLR: collision registry se mazou hned a pri cteni MxPF/PxPF se muze udelat cerstvy render-prepocet.
- Donkey Kong pri mezistavu bez validniho DLISTu nepada do fallback sumu; drzi posledni obraz a tim ma zmizet kratky rozpad mezi intro obrazovkami.
- Obecny XEX core umi pri BRK tesne pred nactenym segmentem pokracovat na nejblizsim nactenem kodu. Cili hlavne loader/trampoline chyby typu Galactic Chase po intru.
- Pacman a dalsi tezke XEXy maji prazdnou OS ROM oblast $C000-$CFFF/$D800-$FFFF jako stub, ne okamzity BRK pad.
- Obecny renderer ma posledni zachranu: kdyz SDLST/HW DLIST pointer neni pouzitelny, opatrne skenuje nactene segmenty na solidni display list.
- Status radek ukazuje core recover BRK, OSROM a scanDL, aby slo z mobilu poznat, jestli tahle zachrana opravdu zabrala.

Chranene reference:

- PiTT-KiTT Remaster zustava nedotknutelny test.
- KiTT Garage zustava druha chranena reference.

Testuj hlavne:

1. PiTT-KiTT Remaster - pismo a hra se nesmi zhorsit.
2. KiTT Garage - barvy, texty a rychlost.
3. Donkey Kong - intro a druhy level.
4. Super Cobra - intro, barvy a jestli FIRE lip zasahuje.
5. Galactic Chase - porad neni prohlaseny za hotovy; pokud po intru stoji, posli ulozeny CORE TEST TXT.
6. De Re Pac-Man - porad neni prohlaseny za hotovy; tenhle titul chce hlubsi OS/CIO/loader emulaci.
