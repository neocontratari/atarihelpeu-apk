# KROK H — čistá TV obrazovka + úspora výkonu pro zvuk

## Co jsem našel v tvém logu

**1) Doostření stojí:** 1,6–4,2 ms u herních scén, ale **7,5 ms**
u filmových (640×480). To byla ta ztráta FPS.

**2) Přenos jede správně:** `bmDrawW=1280 bmDrawH=720`, enkodér na
1280×720, a samotné zabalení do H264 stojí jen 4 ms — takže enkodování
brzda není.

**3) VÁŽNÝ NÁLEZ — zvuk hladoví.** V logu je `underruns=757`.
V tvé původní aplikaci, než jsme začali, tam bylo **0**. Způsobujeme si
to my: náš obraz, doostření a přenos berou procesoru tolik, že na zvuk
nezbývá. **Tohle je ta pravá příčina rozejetého zvuku** — a řešení
nezačíná u zvuku, ale u uvolnění výkonu.

## Co je v tomto balíčku

1. **Čistá TV obrazovka** — LOW/MED/HIGH pryč, výpis FPS a stavu pryč.
   Tlačítko **FULL zůstalo** (celá obrazovka), to se hodí.
2. **Doostření jen u herních scén** — u filmových sekvencí (640×480)
   se přeskočí. Ušetří to 7,5 ms na snímek pro zvuk. Film je stejně
   měkký od přírody, tam doostření tolik nedělá.
3. **FPS v logu** — nový řádek `G TV: X FPS, doostreni Y ms/snimek,
   zdroj 320x240 (doostreno)`. Máme to černé na bílém.

## Verze: EMU10-H-CISTA-TV

## Co mi pošli

1. Je TV obrazovka čistá (bez tlačítek a textu)?
2. Kolik FPS ukazuje log (`G TV: ... FPS`)?
3. **Zlepšil se zvuk?** Je míň rozejetý?
4. Z logu číslo `underruns=...` — mělo by být nižší než 757.

## Co bude dál se zvukem

Podle toho, co ukáže `underruns`, se rozhodneme:
- Když klesne k nule → problém byl ve výkonu, doladíme jen sesouhlasení.
- Když zůstane vysoký → pustíme zvuk do přenosu vlastní cestou přímo
  z jádra, jak jsi navrhoval.
