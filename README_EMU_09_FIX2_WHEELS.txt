EMU-09 FIX2 / Double-line PMG + wheels

Základ: poslední EMU09, ne V10.

Co je opravené oproti EMU09 FIX:
- ponechaný double-line PMG layout PiTT-KiTTu:
  missiles $4180, P0 $4200, P1 $4280, P2 $4300, P3 $4380
- missile 0 se dál čte z low two bits v $4180,Y
- PCOLR2/PCOLR3=$00 se už nebere jako prázdná/debug barva, ale jako skutečná černá
- PMG-only náhled má tmavě šedé pozadí a jemný kontrastní lem kolem černých PMG pixelů, aby černá kola nezmizela na černé
- debug výpis ukazuje PCOLR P0-P3

Test:
1) Load XEX into RAM
2) BOOT GAME / HOLD START
3) RENDER PMG ONLY / REAL COLORS
4) poslat fotku ANTIC/video preview a PMG debug
