# AtariHelp.eu APK Builder — EMU-09 FIX99

FIX99_DONKEY_TRANSITION_COBRA_INPUT_CORE

Tento overlay je určený pro GitHub Desktop. Rozbalit ZIP, obsah složky zkopírovat přes repozitář, commitnout a nechat GitHub Actions postavit APK.

Hlavní směr FIX99:
- Donkey Kong jako chráněná reference: delší potlačení přechodového DLIST $1200, aby se snížil šum mezi intro/menu/level přechody.
- Super Cobra jako chráněná reference: FIRE na mobilním ovládání už nemá omylem držet DOWN/bombu; přísnější guard pro falešné missile zásahy do HUD/fuel oblasti.
- Arkanoid III: čisté menu zůstává.
- Montezuma: VVBLKD fallback zůstává, hráč by měl být vidět; PMG Y jemně doladěné.

Commit summary:
FIX99 Donkey transition Cobra input core
