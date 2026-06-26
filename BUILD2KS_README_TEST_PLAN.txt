AtariHelp.eu EMU-10 BUILD2KS_GTIA_ALL_BITMAP_MODES_DOC_CORE

UCEL:
- KR podle testu sel k horsimu. KN-KQ line-latch/staging take nevyresily Postcard/G2F.
- Audit podle dodane GTIA dokumentace: GTIA mody 9/10/11 nejsou jen ANTIC F; GTIA je aplikuje na jakykoliv ANTIC mod.
- Predtim se GTIA pixel generator aplikoval jen v hires vetvi ANTIC F. Bitmapove ANTIC D/E/A/8/B/C zustaly normalni PF renderer.
- Postcard/G2F proto muze stale tvorit fialove/tyrkysove pruhy mimo spravny GTIA obraz.

ZMENA V KODU:
1) Pridan spolecny GTIA pixel generator gtiaModeColorFromNib() pro GTIA 9/10/11.
2) GTIA9/10/11 se nově pouzije i v m1bpp bitmapach:
   - ctverice 1bit ANx hodnot = 4bit GTIA pixel.
3) GTIA9/10/11 se nově pouzije i v m2bpp bitmapach:
   - dve sousedni 2bit ANx hodnoty = 4bit GTIA pixel.
4) GTIA11 opraveno podle dokumentace:
   - pixel 0 = COLBK hue + jas 0,
   - pixel 1..15 = (COLBK hue OR pixel hue), luma z COLBK.
5) Bez screen-paintu, bez hacku podle nazvu Postcard, bez vlozene bitmapy.
6) KD PMBASE hack zustava pryc.
7) KODY JSOU STEJNE.

TEST:
1) Nahraj overlay pres GitHub Desktop.
2) Build APK.
3) Spust aplikaci.
4) Nahraj test_assets/Postcard_Atari_Rocky.xex.
5) Posli jen:
   - KS posun ano/ne
   - screenshot
   - LOG / CHYBA

V LOGU HLEDEJ:
AtariHelp.eu EMU-10 BUILD2KS_GTIA_ALL_BITMAP_MODES_DOC_CORE pripraven
GTIA BITMAP MODES BUILD2KS
GTIA11 COLOR BUILD2KS
