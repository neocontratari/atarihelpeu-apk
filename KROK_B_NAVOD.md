# KROK B (opraveno) — vložení rendereru dovnitř aplikace

Parťáku, první pokus o krok B zakopl o mou chybu — náš renderer volal
tři funkce, které jsem do balíčku zapomněl přiložit (byly v souboru
s napojením na PS1, který jsem do kroku B schválně nedal). Opravil jsem
to: přidal jsem malý propojovací soubor, který renderer napojí na demo
verzi (barevné pruhy) — přesně to, co v kroku B chceme. Ověřil jsem
u sebe, že ty tři funkce teď existují a všechno se přeloží.

## Co uděláš (celý balíček, jak sis přál)

1. Rozbal ZIP → složka `emu10_B`.
2. Zkopíruj CELÝ obsah přes svůj repozitář → **Nahradit vše**.
3. GitHub Desktop → popisek „krok B oprava" → **Commit** → **Push**.
4. Počkej na build (15–25 min, možná rychleji díky paměti z minula).
5. Actions → Artifacts → `app-debug` → do mobilu.

## Co čekat

Aplikace se chová PŘESNĚ jako Krok A — menu, hry, PS1, cast, všechno
stejně. Náš renderer je uvnitř přibalený a teď se i správně přeloží,
ale ještě se nespouští. Verze se jmenuje „EMU10-B2-RENDERER-VEDLE".

## Co mi pošli

1) Jestli byl build zelený (nebo konec výpisu, kdyby zčervenal).
2) Že se aplikace pořád normálně otevře a spustí PS1 hru.

Když projde: máme potvrzeno, že náš renderer i tvoje aplikace se
v jednom APK snesou. A pak přijde balíček C — výměna PS1 obrazu za
náš plynulý. To hlavní finále.
