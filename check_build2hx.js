const fs = require('fs');
const path = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(path,'utf8');
const req = [
  'BUILD2HX_VBXE_TEXT_LOCAL_ROW_ON_HW',
  'var local=yy+vs;',
  'var textRow=(local>>3);',
  'ATR D1 FAST LOAD BUILD2HW',
  'NET HRY BUILD2HW',
  'BUILD2HP',
  'BUILD2HR',
  'VBXE TEXT XDL RENDER BUILD2HX'
];
let ok = true;
for (const r of req) {
  if (!s.includes(r)) { console.error('MISSING:', r); ok = false; }
}
if (!ok) process.exit(1);
console.log('BUILD2HX check OK: text local row + HW fast ATR/UI markers present.');
