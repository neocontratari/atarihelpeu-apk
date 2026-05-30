# AtariHelp.eu EMU-09 FIX53 PMG ALIGN + COLLISION CORE

FIX53 navazuje na první velký Donkey úspěch z FIX52. Cíl není hack pro jednu hru, ale obecnější XEX jádro:

- obecný ANTIC/DLIST renderer zůstává hlavní cesta pro vlastní XEX,
- PMG overlay má nové generické Y zarovnání pro single-line PMG hry typu Donkey/Kong,
- log vypisuje PMG raw rozsahy a přepočet na obraz (`YRANGE`, `yOffEff`),
- přidána opatrná GTIA collision diagnostika pro obecné XEX (`GTIA COLLISION FIX53`),
- zápis do HITCLR `$D01E` maže collision registry,
- renderer používá DLI/DMA stav i pro výpočet stride/šířky řádků,
- Pitstop/River/PiTT-KiTT zůstávají testy, ne hlavní cíl.

Upload na GitHub: nahraj obsah složky, ne ZIP. V GitHub Desktop napiš Summary, commitni a dej Push origin. APK pak stáhneš z GitHub Actions / Artifacts.
