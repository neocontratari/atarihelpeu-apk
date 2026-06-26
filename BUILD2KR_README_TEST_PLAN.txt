AtariHelp.eu EMU-10 BUILD2KR_G2F_BEAM_EXACT_NO_LATCH_CORE

UCEL:
- Audit po KQ: KQ se nasadil, ale obraz zustal mimo.
- KQ/KP line-latch/staging byly slepa vetev pro Postcard/G2F.
- BUILD2KR vraci G2F na paprskovy renderer podle cyklu, ne na radkovy latch.

ZMENA V KODU:
1) 6502 IO write timestamp opraven na posledni cyklus instrukce:
   - STA abs 4 cykly => zapis na start+3, ne start+4.
   - To posouva GTIA/ANTIC split o realny cyklus zpet.
2) Render snapshot posunut na PF normal start:
   - dokumentace: normal playfield zacina cc=48 = cpu cycle 24.
   - snapAt je tedy lineStart+24, ne +28.
3) Vypnuto neuspesne G2F line-latch/staging z KN-KQ:
   - COLBK/COLPF/COLPM/PRIOR jdou zase pres gtiaRegAt(g,x).
   - PMG HPOS/SIZE/GRAF/GRACTL jdou zase paprskove pres pmRegAt(g,x).
4) Zadny screen-paint, zadny hack podle nazvu Postcard.
5) KD PMBASE hack zustava pryc.

KODY JSOU STEJNE.

TEST:
1) Nahraj overlay pres GitHub Desktop.
2) Build APK.
3) Spust aplikaci.
4) Nahraj test_assets/Postcard_Atari_Rocky.xex.
5) Posli jen:
   - KR posun ano/ne
   - screenshot
   - LOG / CHYBA, kdyz pujde ulozit

V LOGU HLEDEJ:
AtariHelp.eu EMU-10 BUILD2KR_G2F_BEAM_EXACT_NO_LATCH_CORE pripraven
