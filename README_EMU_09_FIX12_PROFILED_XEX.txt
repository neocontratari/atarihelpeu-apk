EMU-09 FIX12 PROFILED XEX

- Vestavěný PiTT-KiTT je znovu vložen z finálního dodaného XEX souboru.
- PiTT-KiTT se pozná i tehdy, když ho uživatel načte ručně jako externí .XEX. Tím se opravuje stav, kdy po STARTu hra běžela a hrál zvuk, ale na obrazovce zůstalo intro.
- Pro PiTT-KiTT se dál používá cílený 2BPP + double-line PMG renderer.
- Cizí XEXy používají obecný ANTIC renderer. Ten je v mobilu opatrně throttlovaný a má rychlejší ImageData cestu pro mode 4/5/fallback.
- Opraveno parsování display list validátoru: $70 je blank 8 scanline, ne LMS.

Poznámka: Super Cobra už ukáže víc, ale pořád může chtít další ANTIC/GTIA/DLI/VBI/ROM přesnosti.
