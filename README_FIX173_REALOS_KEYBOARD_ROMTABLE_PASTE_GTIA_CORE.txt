# AtariHelp.eu EMU-09 FIX173 REALOS KEYBOARD ROMTABLE PASTE GTIA CORE

Overlay navazuje na FIX172/FIX170, ale vraci REAL OS klavesnici na skutecnou XL ROM keyboard tabulku. FIX172 prepisoval KEYDEF/zero-page a tim rozbil BASIC prikazy a TXT paste.

## Zmeny
- Build tag: FIX173_REALOS_KEYBOARD_ROMTABLE_PASTE_GTIA_CORE
- Zruseno synteticke KEYDEF mapovani z FIX172.
- Opravene Atari scan kody podle XL ROM tabulky: `=`, `?`, `(`, `)`, `"`, `<`, `>`.
- CTRL sipky mapovane jako realne CTRL keycodes, ne jako pomichane sipky.
- BASIC TXT / VLOZIT PROGRAM fronta posila cely program v poradi a pumpuje ji jen jednou.
- Teply RESET z FIX172 zustava: program ma zustat po RESETu, maze az NEW.
- GTIA GRAPHICS 9/10/11 z FIX170 zustava v kodu.

## Test plan
1. REAL OS BASIC AUTO BOOT.
2. Otestovat: `PRINT 2+3`, `A=5`, `PRINT A`, `PRINT 1<2`, `PRINT 2>1`, `PRINT (2+3)*4`, `PRINT "AHOJ"`, `PRINT ?` jen jako kontrola znaku.
3. BASIC TXT / VLOZIT PROGRAM s radky obsahujicimi `=`, `?`, `(`, `)`, `"`.
4. RUN, BREAK, RESET, LIST, NEW, LIST.
5. Az klavesnice sedi, GRAPHICS 9/10/11.
