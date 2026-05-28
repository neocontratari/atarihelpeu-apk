EMU-09 FIX / Double-line PMG Overlay

Základ: poslední EMU09 ZIP, bez přebírání V10 past větve.

Opravy:
- opraven JS blok pressStart/tapStartLong/showDisasmHint, který byl v EMU09 syntakticky rozbitý
- PMG renderer už nebere jen single-line layout PMBASE+$300/$400/$500/$600/$700
- pro SDMCTL/DMACTL $2E používá double-line layout:
  missiles = PMBASE+$180
  P0 = PMBASE+$200
  P1 = PMBASE+$280
  P2 = PMBASE+$300
  P3 = PMBASE+$380
- pro PMBASE $4000 tedy:
  missiles $4180, P0 $4200, P1 $4280, P2 $4300, P3 $4380
- missile 0 se čte z low two bits, protože hra zapisuje ORA #$03 do $4180,Y
- přidán PMG debug výpis přímo pod canvas:
  PMBASE, DMACTL, SDMCTL, GRACTL, PRIOR, HPOS, SIZE, nonzero počty a raw Y rozsahy

Test:
1) otevři appku
2) EMU-09 FIX Double-line PMG
3) Load XEX
4) BOOT GAME / HOLD START
5) RENDER PMG ONLY / DOUBLE-LINE nebo RUN VIDEO FRAME
6) pošli fotku canvasu a PMG debug výpisu
