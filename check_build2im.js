const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
const must = [
  'BUILD2IM_VBXE_PRIORITY_XCOLOR_SAFE',
  'M.vbxePfClassFrame=new Uint8Array(384*240)',
  'M.vbxePmMaskFrame=new Uint8Array(384*240)',
  'function vbxeStorePriorityLine',
  'function vbxePriorityAllowsPixel',
  'function vbxePriorityAllowsHiPixel',
  'xcolor?palette[gtia.colpf[1]&0xFF]',
  'BUILD2IF/APK/2IL bez sedeho HL obdelniku'
];
let ok = true;
for (const m of must) {
  if (!s.includes(m)) {
    console.error('CHYBI MARKER:', m);
    ok = false;
  }
}
if (!ok) process.exit(1);
console.log('OK BUILD2IM markers nalezeny.');
