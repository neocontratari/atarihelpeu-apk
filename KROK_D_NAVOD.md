# KROK D — stará grafika zrušena + brzda smyčky

## Co jsem v tvém logu našel

**Náš obraz jel 140–190 FPS!** V logu: 300 snímků za 1,5–2,1 vteřiny.
Vsync na TextureView nebrzdí tak, jak jsem čekal, takže smyčka jela
naplno a překreslovala pořád dokola i snímky, které se nezměnily.
Zbytečně tím žrala výkon, hřála telefon a **brala sílu jádru i castu**
— proto cast stíhal jen 25–29 FPS a obraz po přenosu vypadal hůř.

## Co je opravené

1. **Brzda smyčky.** Kreslíme jen když jádro má nový snímek, a strop
   je 60 FPS. Uvolní se tím hodně výkonu pro emulaci i pro cast.
2. **Stará grafika ZRUŠENA**, jak jsi chtěl. Náš OpenGL obraz je teď
   hlavní zobrazovací cesta pro PS1 a **naskočí sám** při spuštění hry
   — nemusíš na nic klikat. Tlačítko na logu NaP zůstalo jen jako ruční
   přepínač pro testování.

## Verze: EMU10-D-STARA-GRAFIKA-ZRUSENA

## Co mi pošli

- Byl build zelený?
- Spusť PS1 hru — naskočí obraz sám?
- Je cast na TV ostřejší / plynulejší než minule? Kolik FPS ukazuje?
- Log z 8765/log — hledám řádky „C2 bezi" a „D ".

## Co bude dál (krok E) — to, na cos se ptal

Máš pravdu, že poslat obraz na TV **přímo z jádra** bude čistší. Dnes
to jde takhle: hra 640×480 → roztažení na displej (1384×672 s černými
pruhy) → cast vyfotí celou obrazovku → přepočítá → H264. Dvojí zvětšení
= ztráta ostrosti.

Přímo z jádra by to bylo: 640×480 → **jedno** čisté zvětšení na 16:9
(1280×720) → H264. Ostřejší, bez pruhů, nezávislé na tom, co je na
displeji telefonu. Přesně tvoje představa 16:9 na TV.

To je krok E — udělám ho, až tenhle projde.
