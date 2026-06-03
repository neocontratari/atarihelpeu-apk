AtariHelp.eu EMU-09 FIX93 MULTIGAME BOOT + VIEWPORT + DLI CORE

Cíl: nejít hru po hře, ale zlepšit společné jádro pro více Atari 800/XL/XE/130XE XEX souborů.

Změny:
- Opraveno: frame hold se teď opravdu používá v renderVideo(), takže mezi změnami DLISTu se nemá kreslit rozpadlý přechodový obraz.
- Super Cobra: když hra po resetu dočasně vypne NMIEN, ale VBI/DLI vektory jsou platné, renderer/async frame je opatrně obslouží, aby horní text/lišta nemizely do šumu.
- Montezuma added as built-in PMG/viewport test; profil jen pro výškové zarovnání PMG, ne herní hack.
- Montezuma PMG Y offset snížen pro single-line PMG, aby postava/lebka nebyly o patro výš.
- Generic/Montezuma/Arkanoid dostaly jemný delayed boot assist po autostartu, aby hry čekající na START/FIRE/OPTION nezůstaly zdánlivě na PAUZA. Chráněné reference Cobra/Donkey/PiTT/Pitstop/River/Galaxian tím nejsou dotčené.
- Generic trap recovery je jen pro generic/arkanoid/montezuma a jen když už existuje validní DLIST; reference Cobra/Donkey se tím neohýbají.

Test priority:
1. Donkey: zkontrolovat, že zůstal dobrý a šum mezi intry se zmenšil.
2. Cobra: horní text/lišta, scroll, vrtulník, restart, střelba.
3. Montezuma: zda je vidět hráč a lebka sedí výškově.
4. Death Race: zda menu/hra nejsou horší; po START/AUTO poslat snapshot.
5. Galaxian/Galactic Chase: jen kontrolně, protože potřebují hlubší CPU/OS/PMG opravy.
