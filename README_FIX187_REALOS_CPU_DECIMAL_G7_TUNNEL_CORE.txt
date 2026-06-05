FIX187_REALOS_CPU_DECIMAL_G7_TUNNEL_CORE

Navazuje na FIX186. Hlavni oprava neni HSCROL ani clipping: rucni GRAPHICS 7 BASIC program ma legalni souradnice, ale emulator mel spatne decimal SBC pres ADC(~v). Atari BASIC FOR/NEXT a floating point porovnavani pouzivaji decimalni aritmetiku; spatny borrow mohl nechat FOR smycku prejet limit a pak PLOT/DRAWTO spadlo na ERROR-141.

Zmeny:
- opraven 6502 decimal SBC v CPU core
- upraven decimal ADC na konzistentni NMOS-style chovani pro N/Z/V/C
- pridana diagnostika CPU DECIMAL FIX187 do snapshotu
- G7 SPLIT 80 / G7 FULL 96 / G7 TUNNEL 96 zustavaji
- GTIA 9/10/11 BARS zustavaji z FIX184/FIX186
- klavesnice beze zmeny
- zadne herni hacky

Test:
1. Spust REAL OS BASIC.
2. Vloz pres VLOZIT PROGRAM presne Reneho test:
   10 GRAPHICS 7:POKE 752,1
   20 POKE 708,30:POKE 709,148:POKE 710,72
   30 COLOR 1:PLOT 10,10:DRAWTO 70,10
   31 DRAWTO 70,40:DRAWTO 10,40:DRAWTO 10,10
   40 COLOR 2:FOR X=10 TO 70 STEP 4
   50 MX=80-X
   55 PLOT X,10:DRAWTO MX,40
   60 NEXT X
   70 COLOR 3:FOR Y=10 TO 40 STEP 3
   80 MY=50-Y
   85 PLOT 10,Y:DRAWTO 70,MY
   90 NEXT Y
   100 GOTO 100
3. Nesmí skončit ERROR-141 na line 55/85.
4. G7 TUNNEL 96 TEST.
5. GTIA 9/10/11 BARS kontrolne.
