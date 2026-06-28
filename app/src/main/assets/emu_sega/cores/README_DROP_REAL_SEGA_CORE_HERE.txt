AtariHelp.eu EMU-10 / SEGA Mega Drive real core slot
BUILD2MQ_SEGA_ADAPTER_FACTORY_PROBE_STAGE4

Sem patri skutecny Mega Drive core adapter, ne ROM.

ROM soubory (.gen/.bin/.md/.smd) se v appce vybírají pres SBIRKA / HRY / CARTRIDGE SLOT.
Do slozky cores/ patri jen realny emulatorovy JS/WASM adapter.

BUILD2MQ umi automaticky hledat napriklad:
- cores/nap_sega_real_core.js
- cores/sega_real_core.js
- cores/genesis_core.js
- cores/genesis_plus_gx.js
- cores/gpgx.js
- cores/jsmoo_genesis.js
- cores/megadrive_core.js
- cores/md_core.js

Adapter muze byt:
- globalni objekt window.NAP_SEGA_REAL_CORE
- factory window.GenesisPlusGX.create(options)
- constructor/class window.JSMooGenesis

Minimalni API adapteru:
  init({ canvas, width, height, sampleRate, log })  // optional
  loadRom(Uint8Array, info)                        // povinne, nebo loadROM/insertCartridge/loadGame
  press(button) + release(button)                  // optional input
  start() / run()                                  // optional
  reset()                                          // optional

Bez tohoto API appka nesmi kreslit fake obraz.
