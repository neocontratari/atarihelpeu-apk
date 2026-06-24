const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p,'utf8');
const must = [
  'BUILD2IR_G2F_DLI_WSYNC_SAFE',
  'cpu.cycles=target;       // WSYNC = konec aktualni scanline',
  'zadny navrat do stejne radky po WSYNC'
];
let ok = true;
for (const m of must) {
  if (!s.includes(m)) { console.error('CHYBI:', m); ok = false; }
  else console.log('OK:', m);
}
process.exit(ok ? 0 : 1);
