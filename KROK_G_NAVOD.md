# KROK G — přenos na TV v pevných 1280×720 (skutečné 16:9)

## Co jsem našel v logu

Přenos běžel v **1384×672** — to je rozměr tvého displeje na šířku.
Jenže **to není 16:9, ale 2,06:1!** Takže obraz na televizi byl
roztažený ještě o ~16 % víc do šířky, než měl být (obličeje širší),
a televize k tomu sama přidávala pruhy nahoře a dole.

Tvoje "pecka" tedy zatím nebyla poctivé 16:9.

## Co je opravené

Přenos teď posílá **vždycky pevných 1280×720**:
- **skutečné 16:9** — správné proporce, obličeje už nebudou širší
- **bez pruhů** na televizi — vyplní ji celou
- **nezávislé na telefonu** — otočení mobilu už nezmění obraz na TV
- enkodér si rozlišení sám přenastaví (ověřeno v kódu)

## Verze: EMU10-G-TV-1280x720

## Co mi pošli

1. Vypadají proporce správně? (obličeje užší = správně)
2. Zmizely pruhy nahoře a dole na TV?
3. Kolik FPS teď ukazuje HIGH?
4. Vypadá obraz stejně ostře, líp, nebo hůř?

## Co dál (podle toho, co řekneš)

- **Když budou FPS pořád nízké:** zkusíme 960×540. PS1 dává nejvýš
  640×480, takže menší přenos nemusí být vůbec vidět, ale enkodování
  bude o ~44 % levnější → víc FPS.
- **Doostření obrazu:** v tvé aplikaci už jedna taková funkce je,
  dá se použít i tady.
- **Zvuk bokem přímo z jádra:** máš pravdu, že by neměl cestovat
  stejnou cestou jako obraz.
