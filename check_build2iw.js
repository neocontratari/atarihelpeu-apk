const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
const need = [
  'BUILD2IW_VBXE_BCB19_DUAL_DECODE_SAFE',
  'function vbxeDecodeBCB19',
  'function vbxeDecodeBCB21',
  'function vbxeDecodeBCBAuto',
  'VBXE BCB DECODE BUILD2IW',
  'adr=(adr+b.stride)&0x7FFFF'
];
let ok = true;
for (const n of need) {
  if (!s.includes(n)) { console.error('CHYBI: '+n); ok = false; }
}
if (!ok) process.exit(1);
console.log('BUILD2IW kontrola OK');
