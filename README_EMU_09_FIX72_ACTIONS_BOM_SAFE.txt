EMU-09 FIX72 ACTIONS BOM SAFE

Co je nove proti FIX71:

- Opraven build baliku pro GitHub Actions: textove Gradle soubory jsou ulozene jako UTF-8 bez BOM.
- Android versionName je zkraceny na EMU-09-FIX72, aby build nemel zbytecne riziko na metadatech.
- GitHub Actions krok pro vyrobu APK pise podrobnejsi Gradle log a pri padu vypise zakladni diagnostiku projektu.

Jadro emulatoru zustava z FIX71:

- Vestavene testy jsou PiTT-KiTT Remaster, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chranena reference.
- KiTT Garage je druha chranena reference a nespada do PiTT profilu.
- KiTT Garage ma pomalejsi takt, OS shadow barvy se obnovuji pred DLI a ANTIC text 2/3 bere znaky pres COLPF2.
- Super Cobra ma obecnou opravu DLIST validace pro hry s LMS skoro na kazdem radku.
- Obecne PMG hry maji odlozene mazani GTIA kolizi po HITCLR.
- Donkey dostava obecne PMG overlap kolize P/M.

Testuj po uspesnem APK:

1. PiTT-KiTT Remaster - pismo a hra musi zustat v poradku.
2. Donkey Kong - intro a druhy level/bonusy.
3. KiTT Garage - barvy, textove radky a rychlost.
4. Super Cobra - menu/start, barvy a zasahy FIRE.
5. Pitstop II - kontrola, ze se split obraz nezhorsil.
