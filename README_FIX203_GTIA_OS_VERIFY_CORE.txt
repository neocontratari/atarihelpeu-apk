ATARIHELP.EU EMU-09 - FIX203_GTIA_OS_VERIFY_CORE
=================================================

Commit summary:
FIX203 gtia os verify core

Co je ve FIX203:
- Navazuje primo na FIX202_DLI_SCANLINE_VIEWPORT_TRACE_CORE.
- Zadny Donkey-only ani Cobra-only hard-lock.
- XEX loader, DLI/WSYNC trace, real OS/BASIC, G7 testy a zvuk zustavaji z predchozich buildu.
- Tlacitka GTIA 9 / GTIA 10 / GTIA 11 jsou predelana z obycejnych pruhu na OS/BASIC verifier.
- Po kliknuti se nabootuje real OS BASIC a vlozi se Atari BASIC program.
- Program pouziva GRAPHICS 9/10/11, PEEK(88/89) SAVMSC, POKE 623 PRIOR, SETCOLOR/POKE 704-712 a zapisuje 4bit nibbly primo do screen RAM.
- Vzor neni jen pruh: ma dopredny ramp, reverzni ramp, high/low nibble kontrolu a diagonal/checker cast.
- Snapshot ma nove radky:
  - GTIA VERIFY FIX203
  - GTIA VERIFY CODE FIX203
- Cilem je potvrdit, jestli chyba je v OS/BASIC/shadow registrech, v GTIA mapovani barev, nebo az ve hre/DLI/PMG.

Presny test plan:
1. Spust GTIA 9 VERIFY.
   - Pockej, az BASIC program dobehne a obraz zustane stat.
   - Udelej screenshot a snapshot TXT.
2. Spust GTIA 10 VERIFY.
   - Zkontroluj, jestli je videt 9 barevnych zdroju, ne jen nahodne znaky.
   - Screenshot + snapshot TXT.
3. Spust GTIA 11 VERIFY.
   - Zkontroluj 16 hue barev pri stejnem jasu.
   - Screenshot + snapshot TXT.
4. Porovnej stejne programy v Altirre, pokud muzes.
5. Pak teprve test Super Cobra a Donkey Kong, jestli se nic nerozbilo.

Dulezite:
- U techto trech testu cekej dele nez u XEX hry. Je to realny BASIC paste a potom BASIC smycka s POKE do screen RAM.
- Pokud obraz nebude jako Altirra, posli hlavne snapshot radky GTIA VERIFY FIX203, GTIA VERIFY CODE FIX203, GTIA MODE, GTIA RENDER, COLPF/BK HW a PRIOR sh/hw.
- Tohle je diagnosticky krok, ne herni hack. Ma najit spatny zaklad GTIA/OS barev pred dalsim ANTIC/DLI laděnim.
