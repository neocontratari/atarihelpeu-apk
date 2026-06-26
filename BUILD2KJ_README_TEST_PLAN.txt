AtariHelp.eu EMU-10 BUILD2KJ_GTIA9_PMG_MASK_LUMA_CORE

Ucel:
- Navazuje na BUILD2KI.
- Cili na Postcard/G2F rozbite GTIA9/11 + PMG barevne pruhy.
- Nehlasi Night Driver/Decathlon jako hotove.
- KD PMBASE zasah zustava pryc.
- Commando Arnold / Mission / Tetris chraneno bez cilene zmeny.

Zmena v kodu:
1) GTIA barvy pres gtiaPal() ignoruji bit0, jako realny vystup GTIA.
2) GTIA9/11 pixely tvori playfield masku i pri hodnote 0, aby PMG barvici vrstvy netekly pres cely GTIA obraz.
3) Pri GTIA9/11 a neutralnim PRIOR low-nibble=0 se PMG drzi pod GTIA playfield maskou.
4) Neni to hack podle nazvu souboru a neni to screen-paint.

KODY JSOU STEJNE.

Test:
1) Nahrat overlay pres GitHub Desktop.
2) Spustit appku.
3) Spustit test_assets/Postcard_Atari_Rocky.xex.
4) Napsat jen: posun ano/ne.
5) Pokud ne, poslat screenshot + LOG / CHYBA.

Marker v LOGu:
AtariHelp.eu EMU-10 BUILD2KJ_GTIA9_PMG_MASK_LUMA_CORE pripraven
GTIA MODE PFCLASS BUILD2KJ
GTIA PMG MASK BUILD2KJ
