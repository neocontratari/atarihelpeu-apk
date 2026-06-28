BUILD2MO Sega real-core slot

Sem se muze pozdeji vlozit realny Mega Drive JS adapter, napr.:
- nap_sega_real_core.js
- sega_real_core.js
- sega_core.js
- genesis_core.js
- genesis_plus_gx.js

Dulezite: samotny .wasm nestaci. Musi existovat JS/glue adapter, ktery vytvori jeden z globalnich objektu:
NAP_SEGA_REAL_CORE, NAP_REAL_SEGA_CORE, NAP_SEGA_ADAPTER, SegaMDCore, GenesisCore, GenesisPlusGX, GPGX nebo JSMooGenesis.

Minimalni API adapteru:
init({canvas,width,height,sampleRate,log})
loadRom(Uint8Array, info)
press(button)
release(button)
reset/start/pause volitelne

Zadny fake obraz. Bez realneho adapteru zustane status CORE_MISSING.
