EMU-09 FIX75 RELEASE + N&P ICON + DPAD/KBD

Co je nove proti FIX74:

- Ikona aplikace je znovu natvrdo N&P pecet z pecet.png.
- GitHub Actions uz nevyrabi debug APK, ale podepsany release APK app-release.apk.
- Release podpis je stabilni PKCS12 podpis v projektu. Pro uplne verejne sireni je lepsi casem nahradit ho soukromym podpisem pres GitHub Secrets nebo Play Console.
- Manifest nema zbytecne INTERNET opravneni a appka neni debug build. To je nutny krok proti Play Protect varovani, i kdyz reputaci nove sideload APK muze definitivne vyresit az duveryhodny distribucni podpis/obchod.
- WebView je pritazeny: zadny universal access z file URL, zadny mixed content, Safe Browsing na Androidu 8+.
- Mobilni ovladani je prestavene na levy palcovy kriz: nahoru, dolu, vlevo, vpravo.
- FIRE je velke tlacitko vpravo pro pravou ruku.
- Atari klavesnice je presunuta pod levy kriz.
- START zustava uprostred krize a kratke TAP START+FIRE je vpravo nahore.
- PiTT-KiTT textove obrazovky maji cisty background a citelny fallback foreground.
- Super Cobra umi pred ctenim GTIA kolizi cerstve prepocitat PMG overlay a male missile kolize maji sirsi playfield vzorkovani.
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
5. Galactic Chase - jestli po intru uz nespadne na BRK $1888.
6. De Re Pac-Man - jestli je aspon posun proti sumu; tenhle titul muze chtit jeste hlubsi OS/CIO emulaci.
