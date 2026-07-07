BUILD2SA7_STAGE195_BIOS_AUTO_ADOPT_MULTIBIN_CUE_SYNTH (1 soubor: MainActivity)
Aplikuj AZ PO SA6 (dynarec) - stavi na STAGE193+SA6.

1) SONY LOGO + ZNELKA - BIOS AUTO-ADOPT:
   Tvuj log rekl vse: sysdirFiles=[prazdna] - jadro jelo na vestavene nahrade.
   JEDNORAZOVY KROK PRO TEBE: na mobilu stahni BIOS ZIP ze svych stranek
   (page_id=1048) do Download/AtariHelp (nebo Download/AtariHelp/BIOS).
   Appka si pri dalsim spusteni hry BIOS SAMA najde (i v ZIPu), rozbali
   a adoptuje. Markery: PS1_BIOS_ADOPTED count=... / PS1_BIOS_MISSING (navod).
   Pak: sysdirFiles=[scph....bin] a SONY logo + znelka pri bootu hry.
   (BIOS se uklada do privatni slozky, ktere se auto-uklid her NEDOTYKA.)

2) MULTI-BIN HRY (treba 10 .bin bez .cue):
   Rozbali se VSECHNY biny a .cue se SYNTETIZUJE: nejvetsi/Track1 = datovy,
   ostatni AUDIO podle poradi. Hra pojede VCETNE CD hudby.
   Markery: PS1_REMOTE_ZIP_MULTIBIN tracks=N cue=SYNTETIZOVANY / PS1_CUE_SYNTH.
   Pozn.: hry s vlastnim .cue jedou jak drive (to uz fungovalo).

TEST (S8): 1) stahni BIOS ZIP ze stranek do Download/AtariHelp
2) spust jakoukoli hru -> log: PS1_BIOS_ADOPTED + sysdirFiles=[scph...]
   -> SONY LOGO + ZNELKA 3) multi-bin hra (10 binu) -> MULTIBIN tracks=10
   a hudba ve hre hraje. Kdyby cerveny workflow: screenshot staci.
