const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p,'utf8');
const must = [
  'BUILD2JA_XEX_LIVE_MEMAC_PAYLOAD_SAFE',
  'applyXexVbxeLiveMemacPayload',
  'VBXE XEX LIVE MEMAC BUILD2JA',
  'vbxePreloadRangeForCpuAddr',
  'BCB21'
];
let ok = true;
for (const m of must) {
  if (!s.includes(m)) { console.error('CHYBI:', m); ok = false; }
}
if (s.includes('VBXE BCB DECODE BUILD2IW: auto BCB19')) {
  console.error('NESMI OBSAHOVAT destruktivni BUILD2IW BCB19 marker'); ok = false;
}
console.log(ok ? 'BUILD2JA kontrola OK' : 'BUILD2JA kontrola SELHALA');
process.exit(ok ? 0 : 1);
