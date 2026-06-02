# AtariHelp.eu EMU-09 FIX74 TEXT + OSROM + DPAD/KBD

FIX74 reaguje na testy z 2026-06-01 / 2026-06-02:

- Mobilni ovladani je prestavene na levy palcovy kriz, pravy velky FIRE a Atari klavesnice je hned pod krizem.
- PiTT-KiTT Remaster: textove obrazovky maji cisty background a citelny fallback foreground, aby rekord/intro/konec nemizely.
- Super Cobra: PMG/kolize se pri cteni GTIA registru umeji cerstve prepocitat; missile kolize maji sirsi vzorkovani.
- Galactic Chase: obecny BRK recovery umi preskocit z BRK tesne pred nactenym segmentem na nejblizsi nacteny kod.
- Pacman/obecne XEX: prazdna OS ROM oblast $C000-$CFFF/$D800-$FFFF uz neni okamzity BRK pad; status ukazuje OSROM recover count.
- GitHub Actions a Gradle soubory zustavaji UTF-8 bez BOM z FIX72.
- Hlavni vestavene testy jsou PiTT-KiTT Remaster, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chraneny referencni titul.
- KiTT Garage zustava druha chranena reference.
- CORE TEST TXT se uklada do mobilu automaticky a po testu vraci puvodni hru.

Build:

FIX74_TEXT_OSROM_DPAD_KBD


