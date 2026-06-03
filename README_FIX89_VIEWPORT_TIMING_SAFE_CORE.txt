AtariHelp.eu EMU-09 FIX89 VIEWPORT/TIMING SAFE CORE

Cil FIX89:
- nevracet projekt zpatky, ale opravit principy, ktere rozbily mobilni testy,
- resit vice veci najednou: timing, 240px viewport, GTIA collision read, DLI RAM guard a Death Race/OSROM smer,
- nezasahovat do PiTT-KiTT reference a nepretacet emulator do jednoherniho hacku.

Zmeny:
1) Donkey Kong uz nejede omylem PiTT rychlosti 7600.
   Donkey profil je explicitne nastaven na 4200/7ms, protoze na mobilu rychly takt rozhazoval VBI/PMG/kolize u horniho patra.

2) Super Cobra je taky snizena na stabilni 4200/7ms.
   Rychly 5200 takt umel rozjet scroll, ale na mobilu delal divne zasahy/restart/hodni listu.
   Uzivatel muze porad rucne zapnout RYCHLEJI.

3) Zrusen renderVideo() uvnitr cteni GTIA collision registru.
   FIX88/drivejs predtim umel pri cteni $D000-$D00F spustit cely render uprostred CPU smycky.
   Na desktopu to jeste proslo, na mobilu to delalo stiny, falesne kolize a zpomaleni.
   Kolize jsou ted stabilni z posledniho normalne vykresleneho frame.

4) HITCLR zase opravdu maze kolize pro Donkey/Cobra/generic.
   Stare deferovani nechavalo stare stiny moc dlouho. Vyjimku zatim ma River special renderer.

5) DLI RAM guard z FIX88 je vypnuty.
   Death Race a dalsi XEX mohou v DLI samomodifikovat display list/pracovni RAM.
   Plošny guard z FIX88 byl moc hruby. Ochrany jsou ted timing/viewport/scoped OSROM.

6) 240px viewport collision guard.
   U Cobry/Donkey/Galaxian/generic se ignoruje horní/spodní overscan pro collision testy,
   aby rozsireni na 240 radku nevyrabelo smrtici stiny v hornich castich obrazu.

Testovat v poradi:
1. Donkey Kong - horni patro, skok, sud.
2. Super Cobra - scroll, vrtulnik, horni lista, restart, kulomet.
3. Death Race - jestli po menu/intuitivnim startu neni bila rozbita obrazovka.
4. Galaxian - jen kontrolne, jestli se neztratil pokrok.
5. Joystick - pocit zustava jako FIX84/FIX88.
