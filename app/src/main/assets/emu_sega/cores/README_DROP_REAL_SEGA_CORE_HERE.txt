AtariHelp.eu EMU-10 / N&P VISION
BUILD2MP_SEGA_CORE_PICKER_CLARITY_STAGE3

Sem patri REALNY Sega Mega Drive core adapter, ne ROM.

ROM / cartridge:
- .gen / .bin / .md / .smd
- vybira se v appce pres SBIRKA / HRY / CARTRIDGE SLOT

CORE adapter:
- typicky .js glue/adapter + pripadne .wasm
- musi vytvorit globalni adapter, napr. window.NAP_SEGA_REAL_CORE, window.GenesisPlusGX, window.GPGX apod.
- minimalni API: init(canvas/options), loadRom/loadROM/insertCartridge, reset/start/run, press/release

Samotny .wasm bez JS glue nestaci.
V BUILD2MP appka jasne hlasi, kdyz se omylem vybere ROM do CORE pickeru.
