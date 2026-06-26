AtariHelp.eu EMU-10 BUILD2KQ_G2F_ANTIC_EF_RASTER_AUDIT_CORE

CO JE ZMENENE:
- Navazuje na KP/KM stopu, nevraci se na slepe KK/KL.
- Audit podle dodane GTIA dokumentace: G2F/Postcard neni jen ANTIC F + GTIA9/11.
- Postcard v display listu strida ANTIC E/F, proto predchozi KN/KO/KP nechaly cast obrazu porad kreslit svislymi pruhy.
- BUILD2KQ lathuje pro husty ANTIC E/F G2F kernel radkove:
  * COLBK / COLPF / PRIOR
  * COLPM / PRIOR pro PMG barvici vrstvy
  * PMG objektove registry HPOS/SIZE/GRAF/GRACTL
- Bez screen-paintu.
- Bez hacku podle nazvu souboru.
- KD PMBASE zasah zustava pryc.
- KODY JSOU STEJNE.

PROC:
- Dokumentace rika: scanline ma 114 CPU cyklu / 228 color clocks a WSYNC konci u cc208.
- G2F rastr typicky pripravuje barvy po radkach, ne jako svisle pruhy uvnitr obrazku.
- KP jeste aplikoval opravu jen na ANTIC F+GTIA cestu; Postcard ma i ANTIC E radky.

TEST:
1. Rozbal overlay pres GitHub Desktop.
2. Build APK.
3. Spust appku.
4. Nahraj test_assets/Postcard_Atari_Rocky.xex.
5. Porovnej proti Altirre.

OCEKAVANI:
- Svisle fialove/tyrkysove pruhy maji byt znatelne mensi nebo pryc.
- Obraz ma byt bliz Altirre hlavne v hornich oknech a stredovem modrem/zelenem pruhu.

CO POSLAT ZPET:
- Jen kratce: KQ posun ano/ne, pruhy mensi/stejne/horsi.
- Kdyz ne: screenshot + LOG.

LOG MARKERY:
- BUILD2KQ_G2F_ANTIC_EF_RASTER_AUDIT_CORE
- GTIA G2F LINE LATCH BUILD2KQ
- GTIA G2F PM COLOR LATCH BUILD2KQ
- GTIA G2F PMG LINE STAGING BUILD2KQ
