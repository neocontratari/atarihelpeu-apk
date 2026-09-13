# B271 — Oprava zahazování pípnutí + tlačítka bez RETURN

## Nová tlačítka

**"NAPSAT: CSAVE (bez RETURN)"** a **"NAPSAT: CLOAD (bez RETURN)"** —
napíšou jen text. RETURN posíláš ty sám přes samostatné tlačítko RETURN,
přesně jak jsi žádal.

## Skutečná příčina chybějícího pípnutí na začátku

Přesně ověřeno čísly, ne odhadem: po napsání CSAVE a stisknutí RETURN
appka během následujícího 60snímkového bloku (součást psaní příkazu,
kvůli výkonu spuštěného najednou) nahromadí **96 zvukových
přechodů** — to hledané pípnutí! Ale moje vlastní starší oprava
(řešila jiný bug — 67 kliknutí při vstupu do self-testu) tyhle
přechody **vždy zahodí** dřív, než je appka stihne přehrát.

Dvě moje vlastní opravy si odporovaly — jedna zachytává zvuk, druhá
ho maže.

## Oprava

Místo tichého běhu a následného mazání appka teď **průběžně
zachytává zvuk po celou dobu operace** (stejný mechanismus, který už
je ověřený u ukládání WAV) a posílá ho zpátky, aby se **skutečně
přehrál**. Týká se to psaní příkazů, vstupu do self-testu a teď i
bootování — takže "dlouhý zvuk při startu" by teď měl být opravdu
slyšet.

Výhoda tohohle přístupu: žádný konflikt se starší opravou, protože
zvuk se generuje průběžně a žádné "staré nahromaděné přechody" k
zahození vůbec nevzniknou. Vyřešeno u kořene, ne záplatováno.

## Ověření

Všech 10 předchozích testů prochází beze změny + 2 nové testy:
- Simulace nového způsobu psaní CSAVE — pípnutí jasně slyšitelné
  (RMS 0,176, 59 % nenulových vzorků)
- Zachycení zvuku během celého bootu — appka nespadne, zvuk se
  generuje (28 % nenulových vzorků)

## Poctivě: k záhadě s resetem

V logu je RESET zalogovaný jako výslovná, samostatná akce **těsně
před** napsáním CSAVE, ne jako vedlejší efekt CSAVE. Vypadá to na
náhodné zmáčknutí tlačítka, ne na chybu v kódu — ale bez dalšího
potvrzení to netvrdím s jistotou.

## Co zůstává nevyřešeno

- Self-test zvuk/rychlost — samostatná výkonnostní otázka
- Attract mode v praxi na telefonu — pravděpodobně stejná
  výkonnostní příčina

Tohle jsou oddělené otázky od dnešní opravy.
