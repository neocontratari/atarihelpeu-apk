AtariHelp.eu EMU-10 BUILD2KK_FIX_KJ_GPRIOR_MASK_BUG_CORE

KODY JSOU STEJNE.

Co je opravene:
- KJ mel konkretni chybu: v pmShowAt() se GPRIOR/PRIOR orezal na low-nibble pred kontrolou GTIA9/11 bitu $40/$C0.
- Podminka (pr & $C0) proto nikdy nemohla projit.
- Vysledek v logu: pBlockedPF zustalo 0 a PMG barevne vrstvy stale tekly pres GTIA9/11 obraz.
- BUILD2KK bere plny GPRIOR pro detekci GTIA9/11 a low-nibble jen pro prioritu.

Test:
1) Nahrat overlay pres GitHub Desktop.
2) Spustit Postcard_Atari_Rocky.xex z test_assets.
3) Ocekavani: fialove/tyrkysove PMG pruhy pres GTIA obraz maji byt mensi nebo pryc.
4) V logu hledej jen pokud chces: GTIA PMG MASK BUILD2KK a pBlockedPF > 0.
5) Posli: posun ano/ne + screenshot/log.

Netestovat zatim Night Driver/Decathlon, dokud se nepohne Postcard.
