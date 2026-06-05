FIX196_XEX_OS_KERNEL_LOADER_CORE

Proc:
- FIX195 ukazal, ze XEX loader je cisty, ale hry porad bezely jako LEGACY_PROTECTED / QUARANTINE a OS ROM readings byly 0.
- To znamenalo, ze skutecna Atari OS ROM vrstva nebyla pro hry pouzita, jen mini-stuby.
- FIX196 zapina obecnou XEX OS kernel cestu: cisty XEX loader + mapovana Atari XL OS ROM pro hry, ale BASIC direct zustava vypnuty.

Co je v overlayi:
- build tag FIX196_XEX_OS_KERNEL_LOADER_CORE
- zadne nove testovaci pomocne tlacitko
- XEX segment loader/RUNAD/INITAD zustava z FIX195
- pro XEX hry se zapina core XEX_OS_KERNEL misto LEGACY_PROTECTED/quarantine
- optional Atari OS ROM je mapovana pro cteni v $C000-$CFFF a $D800-$FFFF
- BASIC ROM direct pro hotove hry zustava vypnuty
- OS vstupy jako SETVBV/CIOV/SIOV/SYSVBV/XITVBV zustavaji rizene, aby hry nespadly do periferniho cekani
- snapshot pise XEX OS KERNEL FIX196 a pocet OS ROM reads
- FIX187 G7/GTIA/BCD, klavesnice, zvuk a BASIC paste zustavaji

Test:
1. TEST DONKEY KONG XEX - hlavne snapshot: core ma byt XEX_OS_KERNEL, ne LEGACY_PROTECTED/quarantine.
2. TEST SUPER COBRA XEX - musi zustat OK.
3. Montezuma / Arkanoid / Pitstop II / Galaxian jako dalsi generic smoke.
4. U kazdeho poslat screenshot + snapshot, hlavne radky XEX OS KERNEL FIX196, OS STUBS, NATIVE XEX LOADER FIX196.
