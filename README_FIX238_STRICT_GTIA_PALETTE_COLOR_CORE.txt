AtariHelp.eu EMU-09 FIX238 STRICT GTIA PALETTE COLOR CORE
==========================================================

CIL:
- Navazat na FIX237, kde je potvrzene cisty snapshot/key edge/keyclick.
- Opravit barvy v REALOS/PURE HW bez navratu k fake READY obrazovce.
- Vse zustava z raw GTIA/ANTIC/ROM toku: zadne kreslene READY, zadny fake SELF TEST.

ROOT CAUSE:
- FIX236 spravne vypnul vynucene modro-bile READY a ukazal realny stav.
- Tim se projevily dve chyby rendereru:
  1) stary atariColor() byl hruby rainbow odhad, ne strict GTIA byte->RGB mapping.
  2) ANTIC mode 2/3 editor text pouzival COLBK jako pozadi textove plochy, proto byl READY modry na cernem okraji.
     U Atari GRAPHICS 0/editoru je modra textova plocha z COLPF2/COLOR2, zatimco COLBK zustava okraj/border.

ZMENY:
- Prepsan GTIA byte->RGB prevod na FIX238 kalibrovane hue/luma kotvy.
- REALOS ANTIC 2/3 text role:
  - pozadi textove plochy = COLPF2 / COLOR2
  - popredi znaku = COLPF1 / COLOR1
  - COLBK = vnejsi okraj, ne cela editorova plocha
- Snapshot stale uklada jen jeden TXT a prida dukaz:
  FIX238 STRICT GTIA PALETTE CHECK
  RAW2RGB COLPF0 $xx -> rgb(...) | COLPF1 ... | COLPF2 ... | COLBK ...
  RULE STRICT_GTIA_BYTE_TO_RGB / ANTIC2_BG_COLPF2 / ANTIC2_FG_COLPF1 / COLBK_OUTER_BORDER_ONLY / NO_DRAWN_READY_FAKE

POZOR:
- Tohle neni fake modro-bila malovanka. Barvy porad vychazeji z registru COLPF0-3/COLBK.
- Pokud se bude lisit od realneho 130XE/Altirry, dalsi krok je jemna kalibrace tabulky, ne hack obrazovky.

KODY JSOU STEJNE
- BASIC/Altirra porovnavaci kody nebyly meneny.
