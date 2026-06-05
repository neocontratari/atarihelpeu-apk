# AtariHelp.eu EMU-09 — FIX154_REAL_OS_BASIC_BOOT_CORE

## Co je cílem
FIX154 je odpověď na požadavek: žádné kreslené READY a žádné lakování. Přidává oddělený experimentální režim **REAL OS RESET / BASIC BOOT**, kde CPU startuje přes skutečný reset vector z vestavěné `ATARIXL.ROM` a BASIC ROM je mapovaná na `$A000-$BFFF`.

## Důležité
- `TEST REAL ATARI READY` sandbox zůstává v kódu jen jako starý helper, ale tlačítko je odstraněné.
- V UI jsou odstraněná tlačítka:
  - TEST MONTEZUMA ORIG
  - TEST MONTEZUMA ALT
  - ROM NETŘEBA
  - BASIC ROM NETŘEBA
- Nové tlačítko:
  - **EXPERIMENT REAL OS RESET / BASIC BOOT**

## Technické změny
- Build tag: `FIX154_REAL_OS_BASIC_BOOT_CORE`
- Skutečný OS boot je izolovaný od XEX her: `profile=realos`.
- ROM mapování je aktivní jen pro REAL OS boot / ROM_PROBE, ne plošně přes chráněné hry.
- V REAL OS boot režimu se vypínají JS OS/BASIC stub hooky pro CPU PC/JSR do ROM, aby CPU skutečně běžel přes ROM kód.
- BASIC ROM je mapovaná pro čtení CPU v `$A000-$BFFF`.
- XL OS ROM je mapovaná pro `$C000-$CFFF` a `$D800-$FFFF`.
- Obraz v REAL OS boot režimu není ručně kreslený. Pokud se objeví READY/BASIC, musí vzniknout přes emulovanou RAM/DLIST/ANTIC.

## Chráněný baseline
- Donkey Kong zůstává na rychlé cestě bez canvas frameholdu.
- Super Cobra zůstává chráněná.
- Montezuma PRELIM zůstává chráněná.
- FIX149 nepoužívat.

## Commit summary
FIX154 real OS BASIC boot core

## Test plan
1. Spustit APK a ověřit build tag `FIX154_REAL_OS_BASIC_BOOT_CORE`.
2. Kliknout **EXPERIMENT REAL OS RESET / BASIC BOOT**.
3. Nečekat kreslený READY. Pokud READY/BASIC naskočí, je to z ROM bootu. Pokud ne, uložit snapshot.
4. Zkusit přes klávesnici napsat `PRINT 2+2` a ENTER. Pokud BASIC opravdu běží, musí vrátit `4` a READY.
5. Krátce ověřit Donkey Kong, Super Cobra a Montezuma PRELIM, že se nerozbily.

## Poznámka
Tento build neslibuje, že plný Atari OS+BASIC už určitě nabootuje. Slouží k reálnému testu bez falešného READY: buď OS/BASIC vykreslí obraz sám, nebo log ukáže místo pádu/zaseknutí.
