AtariHelp.eu EMU-09 FIX99 DONKEY TRANSITION + COBRA INPUT CORE

Cíl FIX99:
- Donkey Kong je chráněná reference a šum mezi intro/menu/level přechody se řeší strukturálně.
- Super Cobra je chráněná reference; FIRE nesmí omylem vyrobit DOWN/bombu na mobilním kruhovém joysticku.
- Arkanoid menu zůstává čisté z FIX97.
- Montezuma si ponechává VVBLKD fallback a viditelného hráče; PMG Y je jemně doladěné.

Změny:
1) Donkey transition hold
   - DLIST $1200 je označený jako transientní přechodový list.
   - $1200 se nikdy neukládá jako stabilní frame.
   - Při přechodu na $1200 se zachytí poslední čistý canvas a drží se delší dobu.
   - Když $1200 trvá déle, renderer ho potlačí a vrací poslední stabilní frame.

2) Cobra FIRE/down split
   - Když je ve hře Cobra držený FIRE, emulátor maskuje DOWN bit joysticku.
   - DOWN bez FIRE zůstává dostupný pro bombu/special akci.
   - Cíl je odstranit pocit „dvakrát vystřelím a pak to hází bomby“ způsobený mobilním dotykem.

3) Cobra PMG/collision guard
   - Missile PF kolize ignorují horní HUD a spodní fuel oblast přísněji.
   - Falešné slabé zásahy musí mít víc solidních bodů, jinak se ignorují.

4) Montezuma
   - VVBLKD deferred VBI fallback zůstává.
   - Single-line PMG Y offset je 8, aby lebka/hráč seděli víc na cihle.

Test:
- Donkey: menu/intro/level 2 přechod, jestli je šum kratší nebo potlačený.
- Cobra: FIRE bez joysticku dolů, pak DOWN bez FIRE, screenshot + log.
- Montezuma: jestli hráč zůstal vidět a jestli lebka sedí blíž na cihle.
- Arkanoid: menu čisté, hra zatím jen kontrolně.
