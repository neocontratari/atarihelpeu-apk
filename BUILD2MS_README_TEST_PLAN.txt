AtariHelp.eu EMU-10 BUILD2MS_SEGA_LRUSSO_GLOBAL_LEXICAL_BOOT_STAGE6

CIL BUILDU:
- opravit chybu BUILD2MR, kde se online Genesis.min.js stahl, ale adapter hledal pouze window.embedGenesis.
- nektere browser JS core soubory mohou vytvorit global-lexical symbol embedGenesis, ktery neni vlastnost window. BUILD2MS hleda obe varianty.
- pridat graficky core status na 320x224 canvas, aby monitor nebyl jen text. Grafika je diagnostika realneho bootu, ne fake hra.

TEST:
1. Rozbal ZIP pres koren projektu.
2. Spust APK / Nox.
3. Otevri Sega modul.
4. Pres HRY / SBIRKA / CARTRIDGE vyber Sonic nebo Aladdin .gen/.bin/.md.
5. Ocekavane A: pokud se lrusso engine chyti, ma se monitor prepnout z diagnosticke grafiky do realneho Genesis vystupu. Zkus START/A/B/C/D-PAD.
6. Ocekavane B: pokud WebView/engine selze, monitor ukaze grafickou chybu CORE ENGINE LOAD FAILED a LOG popise presny duvod.
7. Klikni ULOZENE a posli AtariHelp_SEGA_LOG_*.txt.

LOG MARKERY:
- BUILD2MS_SEGA_LRUSSO_GLOBAL_LEXICAL_BOOT_STAGE6
- online Genesis.min.js stazeny jako text
- embedGenesis symbol source=window.embedGenesis nebo global-lexical embedGenesis
- ROM_LOAD_PROMISE_PENDING
- ROM_SENT_TO_REAL_CORE_ASYNC / embedGenesis CALLBACK STARTED = nejlepsi vysledek
- ONLINE_SCRIPT_LOADED_BUT_EMBEDGENESIS_MISSING = engine stale nevytvoril pouzitelny symbol

POCTIVE:
- ClownMDEmu od Clownacy je zajimavy C core kandidat, ale neni to hotovy WebView JS adapter. Vyuzitelne spis pro dalsi native/JNI/NDK nebo WASM cestu.
- Tento build stale nepridava ROM do APK a nesmi kreslit fake Sonic gameplay.
