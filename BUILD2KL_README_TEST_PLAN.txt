AtariHelp.eu EMU-10 BUILD2KL_WSYNC_HBLANK_GTIA11_G2F_CORE

ZAKLAD:
- Navazuje na BUILD2KK.
- KK log ukazal, ze PMG uz bylo blokovane (pDraw=0, pBlockedPF>0), ale obraz se nezmenil.
- Proto dalsi rez neni dalsi PMG maska, ale zakladni GTIA/G2F raster.

ZMENA V KODU:
1) WSYNC/HBLANK:
   - WSYNC uz neskace slepe az na konec cele scanline.
   - CPU se po WSYNC pusti do HBLANK casti radky kolem cyklu 104.
   - DLI/G2F kernel tak muze pripravit COLPF/COLBK/PRIOR pro dalsi scanline vcas.

2) GTIA11 barva:
   - Opravena formule GTIA11:
     pixel 0 = COLBK hue + luminance 0,
     pixel 1-15 = (COLBK hue OR pixel_value) + COLBK luminance.
   - Neni to hack na Postcard, je to obecny GTIA rezim 11.

CO NENI MENENO:
- KD PMBASE zasah je pryc a zustava pryc.
- VBXE blitter/Commando Arnold cesta se nemenila.
- UI, kazeta, joystick, loader beze zmen.
- KODY JSOU STEJNE.

TEST:
1) Nahraj overlay pres GitHub Desktop.
2) Spust appku.
3) V logu musi byt:
   AtariHelp.eu EMU-10 BUILD2KL_WSYNC_HBLANK_GTIA11_G2F_CORE pripraven
4) Otestuj pouze:
   test_assets/Postcard_Atari_Rocky.xex
5) Posli vysledek:
   posun ano/ne
   pokud ne: screenshot + LOG

LOG MARKERY:
- WSYNC HBLANK BUILD2KL
- GTIA11 COLOR BUILD2KL
