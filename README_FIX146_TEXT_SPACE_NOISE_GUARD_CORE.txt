AtariHelp.eu EMU-09 - FIX146_TEXT_SPACE_NOISE_GUARD_CORE

Cíl buildu:
- navázat na rychlostní zisk FIX144/FIX145;
- NEvracet canvas framehold/getImageData/putImageData;
- začít řešit obecný textový/charset šum, který se objevuje u více her, ne jen u Donkey Kongu;
- zachovat Montezuma PRELIM opravu z FIX140;
- zachovat Super Cobra jako hratelnou referenci;
- zachovat Donkey FIX69/$466F rychlou cestu.

Změny:
1) Obecný text-space noise guard
- U klasických ANTIC textových režimů 2/3/6/7 se kontroluje, jestli znak 0/mezera v aktuálním CHBASE není omylem plný bodů.
- Pokud je mezera v charsetu podezřele tečkovaná, renderer ji vykreslí jako čisté prázdné místo.
- Cíl: méně teček/čaje kolem textu a v menu obrazovkách napříč více hrami.
- ANTIC 4/5 zatím zůstává opatrně beze změny, protože tam může být znak 0 skutečný grafický tile.

2) CHBASE cache per frame
- charBaseGuess už neprovádí drahé countNonZero(1024) pro každý znak a každou scanline znovu.
- Výsledek se cachuje per frame/profile/CHBASE.
- Cíl: udržet rychlost z FIX144/FIX145 a pomoci i mobilnímu WebView.

3) Donkey Kong
- Zůstává bez canvas frameholdu.
- Zůstává FIX69/$466F cesta z FIX145.
- Tenhle build nezkouší další Donkey-specific přikrývání šumu.

4) Montezuma / Super Cobra
- Montezuma PRELIM z FIX140 zůstává zamčená.
- Super Cobra zůstává chráněná hratelná reference.

Test plan:
1. Donkey Kong original: rychlost + šum mezi intry/menu/hrou.
2. Donkey Kong Arcade.xex: intro stabilita / hra rozsekaná nebo ne.
3. Donkey Kong Junior Title Version: intro + začátek hry.
4. Montezuma PRELIM: musí dál fungovat.
5. Super Cobra: musí dál být hratelná.
6. Arkanoid III: menu a textové tečky, i když hru tenhle build ještě nemusí opravit.

Hledat v logu:
BUILD TAG FIX146_TEXT_SPACE_NOISE_GUARD_CORE
FIX146 CORE
textNoise=
