AtariHelp.eu EMU-09 FIX235 PURE HW TRIG3 POKEY SIO CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy nejsou zmenene.

Proc FIX235:
- FIX234 audit byl cisty, ale BASIC porad padal do self-test RAM smycky $532A-$533D.
- OPTION SELF TEST dosel do SIO/POKEY cekani $EA9E a na WebAudio z toho vzniklo trvale piskani.
- V logu bylo videt, ze POKEY registr D200-D208 se latcheval, ale stara audio bridge z nej delala browser oscillator.

Zmeny:
- PURE HW TRIG3 $D013: vraci realny stav externi cartridge linky. V PURE HW neni externi cartridge vlozena, internal BASIC se ridi PORTB bit1.
- PURE HW D011/D012 zustavaji normalni trigger idle.
- PURE HW POKEY SEROUT $D20D: pridan serial-ready timing event, aby ROM SIO kod nevisel ve smycce $EA9E vecne. Nejde o skok PC a nekresli se zadna obrazovka.
- PURE HW SIO no-device timer: kdyz ROM zustane dlouho v $EA9E/$EAA0/$EAA5/$EAA7, vyprsi timer stav $11 a ROM sama vezme timeout vetev.
- PURE HW POKEY audio: behem bootu se D200-D208 jen latcheuji do registru, ale stary jednoduchy browser oscillator se nespousti. Tim se odstrani trvale piskani z boot smycky.
- Snapshot doplnen o FIX235 ROOT CHECK, TRIG3/CART, POKEY/SIO, AUDIO, SIO LOOP COUNT.

Test:
1. PURE HW AUDIT + SNAPSHOT
2. POWER XL/XE BASIC + SNAPSHOT
3. POWER OPTION SELF TEST + SNAPSHOT

Sleduj hlavne:
- PURE HARDWARE SNAPSHOT FIX235
- TRIG3/CART
- POKEY/SIO
- AUDIO
- jestli BASIC uz nejde do $532A-$533D
- jestli OPTION SELF TEST uz nepiska a neopakuje $EA9E vecne
