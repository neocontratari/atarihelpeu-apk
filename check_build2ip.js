const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
const checks = [
  'BUILD2IP',
  'vbxeGraphLocalY',
  'vbxeGraphSourceAddr',
  'PF3 nechava globalni',
  'VBXE XDL ATTRIBUTE MAP APPLY BUILD2IP',
  'VBXE HR/SR XDL RENDER BUILD2IP',
  'BUILD2IP_VBXE_OVSCROLL_MAP_PF3_SAFE'
];
let ok = true;
for (const c of checks) {
  if (!s.includes(c)) { console.error('CHYBI:', c); ok = false; }
  else console.log('OK:', c);
}
process.exit(ok ? 0 : 1);
