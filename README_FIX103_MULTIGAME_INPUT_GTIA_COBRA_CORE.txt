# EMU-09 FIX103 MULTIGAME INPUT + GTIA/COBRA CORE

Cíl: vrátit směr z herních hacků zpět ke společnému jádru emulátoru.

- Donkey Kong zůstává na bezpečném FIX100/FIX102 baseline bez dlouhého $1200 holdu.
- Cobra title/menu fallback zůstává, protože je čitelný, ale gameplay řeší obecnější input/GTIA core.
- FIRE na mobilu dává opakované TRIG0 hrany pro continuous-fire hry místo falešného joystick DOWN.
- GTIA player/playfield kolize jsou pro Cobru znovu zapnuté podle playfield pixelů, aby tanky/terén nebyly průchozí.
- Missile/playfield test je méně agresivně potlačovaný, aby střely a bomby nebyly schované dřív než dopadnou.
- Arkanoid menu a Montezuma VVBLKD/PMG viditelnost zůstávají zachované.

Testuj hlavně: Cobra menu, gameplay hory/barvy, opakované bomby při drženém FIRE, náraz do tanku/terénu; potom rychle Donkey/Arkanoid/Montezuma regresi.
