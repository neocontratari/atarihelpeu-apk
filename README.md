AtariHelp.eu EMU-09 FIX95 DLIST STABILITY + SCAN CORE

Pokračování po testu FIX94. Donkey Kong zůstává chráněná reference, ale FIX95 jde po strukturálním jádru: stabilní DLIST snímky, lepší výběr runtime DLISTu u Cobry, neopakovat automatické přeskočení Arkanoid menu a opravený SNAPSHOT log.

Cíl: zmenšit šum mezi Donkey intry bez uložení špatného DLIST $1200 jako stabilního, vrátit Cobru z nízkého $2000 menu/shadow DLISTu k lepšímu naskenovanému runtime DLISTu a získat čisté logy bez ReferenceError.

René workflow: rozbalit ZIP overlay do projektu přes GitHub Desktop, Commit Summary použít z README_FIX95_DLIST_STABILITY_SCAN_CORE.txt a poslat screenshoty/logy podle test plánu.
