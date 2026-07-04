AtariHelp EMU-10 BUILD2SA5N STAGE163

Zamer:
- PS1 zvuk se po case porad kouse.
- L1/L2/R1/R2 maji byt dole u palcu, ne nahore.

Zmeny PS1 ovladani:
- L1 a L2 presunuty dolu mezi D-pad a SELECT.
- R1 a R2 presunuty dolu mezi START a prava akcni tlacitka.
- Long-press lock zustava stejny:
  - podrzet L1/L2/R1/R2 = zamknout jako drzeny,
  - dalsi tap = odemknout.

Zmeny PS1 audio:
- Java AudioTrack tahne PS1 audio v blocich 735 frames, tedy prirozene cca 1/60 s pri 44.1 kHz.
- S8/no-starve profil ma vetsi reservoir:
  - wantedFrames 12288
  - prefillTarget 8192
  - delsi prefill deadline
- Retry pri kratkem pullu je jemnejsi a castejsi.
- Native FIFO minPull zvysen na 256 frames, aby Java netahala drobky.
- PS1 JPEG preview zpomaleno z ~20 fps na ~12.5 fps, porad JPEG95, ale s prioritou pro audio/core.

Proc:
- Log ukazuje, ze audio FIFO se neplni do runaway backlogu.
- Pozdeji ale skutecny frame progress klesa pod stabilnich 60 fps, takze se audio po case muze kousat kvuli vykonu/teplu/interpreter jadru.
- Tahle etapa snizuje WebView/JPEG zatez a zarovnava audio na prirozeny PS1 takt. Neni to dynarec, ale je to nejrozumnejsi dalsi stabilizace bez rozbijeni jadra.

Zamerne NEzmeneno:
- emu_sega/index.html neni v baliku.
- Sega native core neni v baliku.
- 130XE core/assets nejsou v baliku.

Test:
- Medal of Honor / NFS / Diablo: nechat bezet dele, nez se zvuk obvykle zacal kousat.
- Otestovat L1/L2/R1/R2 dole palci v landscape.
- V logu hledat BUILD2SA5N PS1_AUDIO_START a PS1_AUDIO_WRITE.
