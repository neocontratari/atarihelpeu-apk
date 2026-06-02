AtariHelp.eu EMU-09 FIX83 COBRA ASYNC SAFE + GALAXIAN VJOY

Základ: FIX82/FIX81/FIX77 stabilní větev.

Cíl FIX83:
- zachovat citlivý kruhový joystick z FIX82, který podle testu funguje dobře v Donkey;
- nevracet se zpět, ale opravit regresi Super Cobra;
- Galaxian INIT-only/DLI ochranu držet jen pro profil Galaxian, ne pro všechny INIT-only XEXy.

Důvod opravy:
Ve FIX82 byla proměnná galaxSafe aktivní i pro všechny INIT-only XEXy bez RUNAD. Super Cobra je také INIT-only, takže dostala Galaxian RTI/stack wrapper. To mohlo rozbít VBI/DLI timing, scroll krajiny a PMG vrtulník.

FIX83 mění:
- runAsyncAtariRoutine(): galaxSafe = isGalaxianProfile() pouze;
- Cobra/Super Cobra znovu používá původní FIX77/FIX81 async cestu;
- Galaxian INIT-only BRK idle a DLI/RTI ochrana zůstává izolovaná jen na Galaxian;
- joystick beze změny, bez timeru.

Test pořadí:
1. Super Cobra: musí rolovat krajina a být vidět vrtulník.
2. Donkey Kong: musí zůstat dobrý jako FIX82.
3. Joystick: citlivost ponechána jako FIX82.
4. Galaxian: zkontrolovat, že dál kreslí a nepřestal běžet.
