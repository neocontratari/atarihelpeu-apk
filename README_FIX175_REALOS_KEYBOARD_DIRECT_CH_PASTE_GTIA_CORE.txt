# AtariHelp.eu EMU-09 FIX175 REALOS KEYBOARD DIRECT CH PASTE GTIA CORE

Overlay pro GitHub Desktop. Navazuje na FIX174/FIX170, ale hlavni oprava je REAL OS/BASIC klavesnice.

## Commit summary

FIX175 realos keyboard direct ch paste gtia core

## Co je opravene

- Build tag: `FIX175_REALOS_KEYBOARD_DIRECT_CH_PASTE_GTIA_CORE`.
- Rovnitko je vracene na skutecny Atari OS CH kod `$0F`.
- Vetsi nez `>` zustava na CH kodu `$37`.
- Otaznik `?` jde jako SHIFT+/ CH kod `$66`.
- Zavorky, uvozovky a dalsi SHIFT symboly jsou v jedne prime ROM tabulce, bez KEYDEF prepisu.
- BASIC TXT / VLOZIT PROGRAM cisti starou frontu pred novym pastem a posila znaky pomaleji.
- LIST / NEW / teply RESET zustava z predchozi opravy.
- GTIA 9/10/11 kod z FIX170 zustava zachovany, ale hlavni test FIX175 je klavesnice.

## Test plan

1. Spust REAL OS BASIC AUTO BOOT.
2. Otestuj samostatne: `=`, `>`, `?`, `(`, `)`, `"`.
3. Otestuj prikazy:

```basic
PRINT 2+3
LET A=5
PRINT A
PRINT 2>1
? "AHOJ"
PRINT (2+3)*4
```

4. Otestuj BASIC TXT / VLOZIT PROGRAM na kratkem programu s `=` a `?`.
5. Potom zkus GRAPHICS 7 jako kontrolu a teprve pak GRAPHICS 9/10/11.

Poznamka: `A=5` v primem rezimu muze byt podle BASIC parseru citlive; pro overeni rovnitka pouzij hlavne `LET A=5` a radkovany program `10 A=5` / `20 PRINT A`.
