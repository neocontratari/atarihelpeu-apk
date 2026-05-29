EMU-09 FIX26 UNIVERSAL DLI/PMG CORE

PiTT-KiTT zustava referencni XEX. FIX26 neopravuje River Raid rucnim patchem pro jednu hru, ale posiluje obecne jadro: DLI se spousti podle display-list scanline bitu $80, stav GTIA/PMG registru se uklada po DLI, renderer pouziva tyto stavy pro scanline PMG overlay a cizi hry dostavaji korektnejsi TRIG1-3 vstupy pred collision registry.
