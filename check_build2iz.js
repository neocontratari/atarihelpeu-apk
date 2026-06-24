const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p,'utf8');
const must = [
  'BUILD2IZ_XEX_INITAD_MEMAC_PRELOAD_SAFE',
  'annotateXexVbxeMemacPreload',
  'applyXexVbxePreloadRecord',
  'VBXE XEX MEMAC PRELOAD BUILD2IZ',
  'BCB21'
];
let ok = true;
for (const m of must) {
  if (!s.includes(m)) { console.error('CHYBI:', m); ok = false; }
}
if (s.includes('VBXE BCB DECODE BUILD2IW: auto BCB19')) {
  console.error('NESMI OBSAHOVAT destruktivni BUILD2IW BCB19 marker'); ok = false;
}
console.log(ok ? 'BUILD2IZ kontrola OK' : 'BUILD2IZ kontrola SELHALA');
process.exit(ok ? 0 : 1);
