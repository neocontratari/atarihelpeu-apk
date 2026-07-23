# KROK F2 — oprava: obraz na TV se neodevzdával

## Co bylo špatně (moje chyba)

V kroku F jsem obraz z jádra poctivě připravil do bitmapy, ale
**zapomněl jsem ji odevzdat do přenosu** — chyběl jediný řádek
`napTvWebPublishBitmap(...)`. Televize proto pořád čekala na starou
prázdnou bitmapu → černo a 0 FPS. Zvuk jel, protože ten jde jinou
cestou. A log spamoval, protože se přenos pořád marně dožadoval snímku.

Doplněno.

## Co v F2 máš

1. **Obraz na TV přímo z jádra** — jedno čisté roztažení na 16:9,
   bez černých pruhů a bez ovládacích prvků.
2. **Automatický poměr stran** — na výšku 4:3, na šířku 16:9.
3. **Tlačítko na logu zrušeno** — vyskočení z PS1 máš volné
   (to už jsi potvrdil, že funguje).

## Verze: EMU10-F2-TV-PRIMO-Z-JADRA

## Co mi pošli

1. Je vidět obraz na TV, když běží PS1?
2. Je ostřejší než dřív? Zmizely černé pruhy a ovládací prvky?
3. Kolik FPS ukazuje cast dole v liště?
4. Je zvuk blíž obrazu?
5. Přestal log spamovat?
