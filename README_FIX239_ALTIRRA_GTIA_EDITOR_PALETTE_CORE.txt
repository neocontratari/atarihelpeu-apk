AtariHelp.eu EMU-09 FIX239_ALTIRRA_GTIA_EDITOR_PALETTE_CORE

CIL:
- Doladit posledni barevny offset po FIX238.
- Zachovat cisty PURE HARDWARE/REALOS tok: zadne kreslene READY, zadny fake SELF TEST, zadne herni hacky.
- Raw GTIA registry zustavaji zdrojem pravdy; meni se jen byte->RGB prevod pro WebView canvas.

ROOT CAUSE Z FIX238 LOGU:
- BASIC GRAPHICS 0 mel spravne raw registry:
  COLPF0=$28 COLPF1=$CA COLPF2=$94 COLPF3=$46 COLBK=$00.
- FIX238 uz spravne nastavil ANTIC text role:
  bg=COLPF2, fg=COLPF1, COLBK jen vnejsi border.
- Chyba byla v hue anchoru: $CA se mapovalo do zeleneho RGB misto do jasne cyan/bile Atari barvy.

ZMENY:
- Pridan FIX239 Altirra-like GTIA palette calibration.
- $CA mapovano na jasne bile/cyan pismo rgb(232,247,255).
- $94 mapovano na sytou Atari modrou rgb(75,119,190).
- Ostatni hue anchors upraveny plynule, porad podle raw GTIA bajtu.
- Snapshot je porad single TXT.
- Klavesnice, RETURN a POKEY keyclick z FIX237 zustavaji zachovane.

SNAPSHOT MUSI OBSAHOVAT:
BUILD FIX239_ALTIRRA_GTIA_EDITOR_PALETTE_CORE
FIX239 ALTIRRA GTIA EDITOR PALETTE CHECK
RAW2RGB COLPF0 ... COLPF1 $CA -> rgb(232,247,255) ... COLPF2 $94 -> rgb(75,119,190) ...
VERIFY BASIC DEFAULTS COLPF1 $CA rgb(232,247,255) / COLPF2 $94 rgb(75,119,190)
RULE ALTIRRA_LIKE_GTIA_RGB_CALIBRATION / ANTIC2_BG_COLPF2 / ANTIC2_FG_COLPF1 / RAW_BYTES_PRESERVED / SNAPSHOT_SINGLE_TXT / NO_DRAWN_READY_FAKE

TEST PLAN:
1. POWER XL/XE BASIC.
2. Porovnat obraz s real Atari 130XE a Altirrou: pozadi modre, pismo jasne bile/cyan.
3. ? FRE(0) musi zustat 37902.
4. Jedno pismeno = jeden znak.
5. RETURN = jeden radek.
6. SNAPSHOT = jeden TXT.
7. POWER OPTION SELF TEST + snapshot.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou zmenene.
