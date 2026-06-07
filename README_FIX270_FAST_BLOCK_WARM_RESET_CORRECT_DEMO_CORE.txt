FIX270_FAST_BLOCK_WARM_RESET_CORRECT_DEMO_CORE

Forward oprava z funkcniho FIX267/FIX261. Nejde o rollback a neprebira rozbite FAST/POKEY pokusy z FIX268/269.

Dulezite z testu: u AHOJ PARTAKU se v predchozim helperu demo fronta koncila radkem 20 GOTO 20. To zpusobi jeden vypis a pak tichy nekonecny skok na radek 20. FIX270 nastavuje default na 20 GOTO 10.

Zmeny:
- RESET na virtualni klavesnici je WARM RESET BASIC, ma zachovat BASIC RAM/program.
- Pridany samostatny COLD RESET BASIC.
- FAST BASIC jde pres radkove ATASCII/KGETCH bloky po real BASIC READY.
- Po EOL $9B se ceka mezi radky, aby BASIC editor/tokenizer stihl radek ulozit.
- Stare screen RAM / RAW CH / slow real-key / rozbite pokusy se pri nove fronte vypinaji.
- Bez fake READY, fake LOAD, RAM/program injectu a bez screen RAM write.

Test:
1. POWER XL/XE BASIC.
2. V textarea musi byt:
   10 PRINT "AHOJ PARTAKU"
   20 GOTO 10
3. VLOZIT FAST + RUN.
4. Cekany vysledek: nekonecne se vypisuje AHOJ PARTAKU.
5. Klavesa RESET = WARM RESET BASIC. Po READY zkus LIST, program ma zustat.
6. COLD RESET BASIC = cisty BASIC boot.
