# AtariHelp.eu EMU-09 FIX73 DPAD + BRK + COBRA + GAL/PAC

FIX73 reaguje na testy z 2026-06-01 / 2026-06-02:

- Mobilni ovladani je prestavene na levy palcovy kriz a pravy velky FIRE.
- Super Cobra renderuje PMG/kolize kazdy frame a male missile kolize maji sirsi playfield vzorkovani.
- Galactic Chase: obecny BRK recovery umi preskocit z BRK tesne pred nactenym segmentem na nejblizsi nacteny kod.
- Pacman/obecne XEX: pokud SDLST/HW DLIST pointer selze, renderer opatrne skenuje nactene segmenty na solidni display list.
- GitHub Actions a Gradle soubory zustavaji UTF-8 bez BOM z FIX72.
- Hlavni vestavene testy jsou PiTT-KiTT Remaster, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chraneny referencni titul.
- KiTT Garage zustava druha chranena reference.
- CORE TEST TXT se uklada do mobilu automaticky a po testu vraci puvodni hru.

Build:

FIX73_DPAD_BRK_COBRA_GAL_PAC


