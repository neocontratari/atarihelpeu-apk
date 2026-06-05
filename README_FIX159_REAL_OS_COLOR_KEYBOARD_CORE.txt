AtariHelp.eu EMU-09 FIX159 REAL OS COLOR KEYBOARD CORE

Commit summary:
FIX159 real OS color keyboard core

Co je v overlayi:
- navazuje na FIX158, kde se poprvé objevil skutečný OS/BASIC READY bez kresleného sandboxu,
- žádné falešné READY a žádný fake BASIC,
- REALOS textová obrazovka má rendererovou korekci barev: modré pozadí + světlý text,
- opravené opakování kláves: jeden klik/keydown má do CH $02FC projít jen jednou,
- hardware key repeat je v REALOS/READY režimu ignorovaný,
- mobilní Atari klávesnice je přestavěná do QWERTY/Atari rozložení: čísla, QWERTY řady, SPACE, DEL, RETURN,
- XEX hry zůstávají na chráněném baseline: Donkey/Cobra/Montezuma se nepřepínají do REAL OS režimu.

Test plan:
1. Klikni REAL OS BASIC AUTO BOOT.
2. Ověř, že READY zůstává a obraz má modré Atari pozadí.
3. Zkus jednou stisknout B: nesmí vysypat 30 písmen.
4. Zkus RETURN: nesmí odskočit o mnoho řádků.
5. Zkus PRINT 2+2 a RETURN, pokud BASIC už přijímá znaky.
6. Krátce ověř Donkey Kong, Super Cobra a Montezuma PRELIM, že baseline není horší.

Poznámka:
FIX159 pořád není finální plný Altirra-level OS boot. Je to další poctivý krok po FIX158: READY/BASIC obraz už vzniká z emulovaného OS stavu, ale klávesnice a barevná interpretace se teď stabilizují.
