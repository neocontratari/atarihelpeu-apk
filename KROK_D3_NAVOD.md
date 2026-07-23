# KROK D3 — oprava mojí chyby

Parťáku, ta chyba byla moje, ne tvoje ani GitHubu.

Když jsem do balíčku D2 přidával tu "drobnou změnu", napsal jsem
poznámku ve špatném formátu: použil jsem `//`, ale soubor eglrender.cmake
používá `#`. Tím jsem ten soubor rozbil a build spadl s hláškou
"Parse error ... got unquoted argument with text //".

Opraveno. Navíc jsem projel všechny podobné soubory, aby se to
neopakovalo.

## Obsah je jinak stejný jako krok D

1. **Brzda smyčky** — kreslíme jen když má jádro nový snímek, strop
   60 FPS. (V logu jel náš obraz 140–190 FPS a bral výkon jádru i castu.)
2. **Stará grafika zrušena** — náš OpenGL obraz je hlavní cesta pro PS1
   a naskočí sám při spuštění hry.

## Verze: EMU10-D3-STARA-GRAFIKA-ZRUSENA

## Postup

Rozbal → zkopíruj CELÝ obsah přes repozitář → **Nahradit vše** →
Commit „krok D3" → Push → počkej na zelenou → APK do mobilu.

## Co mi pošli

- Byl build zelený?
- Spusť PS1 hru — naskočí obraz sám, bez klikání na logo?
- Je cast na TV ostřejší a kolik FPS ukazuje (dole v liště)?
- Log z 8765/log — hledám „C2 bezi" a „D ".
