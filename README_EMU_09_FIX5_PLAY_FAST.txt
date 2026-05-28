EMU-09 FIX5 PLAY FAST

Navazuje na ověřený EMU09 FIX4.

Změny:
- debug PMG DOM se při hraní neaktualizuje každý snímek
- playfield 2BPP se kreslí přes ImageData framebuffer, ne tisíci fillRect volání
- CPU smyčka má časový limit na snímek, aby WebView nezamrzal
- kratší bootovací doběh po START
- přidána tlačítka ÚSPORNĚ / NORMÁL / RYCHLEJI a přepínač VIDEO FAST/SAFE

Základ PMG zůstává: double-line PMBASE $4000, missiles $4180, P0 $4200, P1 $4280, P2 $4300, P3 $4380.
