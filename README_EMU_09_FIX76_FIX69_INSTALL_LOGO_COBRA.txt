EMU-09 FIX76 FIX69 INSTALL + N&P LOGO + COBRA

Co je nove proti FIX75:

- Ikona aplikace je znovu natvrdo N&P pecet z pecet.png.
- N&P logo je znovu v assets jako np_logo.png a EMU-09 obrazovka ho ukazuje primo nahore.
- GitHub Actions jsou vratene na overeny FIX69 styl: assembleDebug a artifact HOTOVE_APK_ATARIHELP_STAHNI_ME / app-debug.apk.
- Manifest je take vraceny smerem k FIX69: INTERNET opravneni a allowBackup=true, bez roundIcon. To je kvuli tomu, ze FIX69 se na telefonu overene instaloval lepe nez FIX75 release pokus.
- Mobilni ovladani je prestavene na levy palcovy kriz: nahoru, dolu, vlevo, vpravo.
- FIRE je velke tlacitko vpravo pro pravou ruku.
- Atari klavesnice je presunuta pod levy kriz.
- START zustava uprostred krize a kratke TAP START+FIRE je vpravo nahore.
- PiTT-KiTT textove obrazovky maji cisty background a citelny fallback foreground.
- Super Cobra ma uzsi missile kolizi: playfield zasah se bere hlavne ze spicky rakety, ne z velkeho nafouknuteho obdelniku.
- Super Cobra FIRE z velkeho mobilniho tlacitka je kratky puls, aby drzeni tlacitka nevypadalo jako nesmyslna strelba.
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
