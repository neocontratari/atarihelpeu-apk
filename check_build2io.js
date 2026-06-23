const fs = require('fs');
const path = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(path,'utf8');
const checks = [
  'BUILD2IO_VBXE_XDL_MAPATTR_AUDIT_SAFE',
  'XDLC_MAPADR = 19bit AMAP',
  'function vbxeMapAttrAtHi',
  'VBXE XDL ATTRIBUTE MAP APPLY BUILD2IO',
  'mapOn:!!st.mapOn',
  'vbxeApplyAttributeMapToAtari(recs)'
];
let ok = true;
for (const c of checks) {
  if (!s.includes(c)) { console.error('CHYBI:', c); ok = false; }
  else console.log('OK:', c);
}
if (!ok) process.exit(1);
console.log('BUILD2IO kontrola OK - XDL MAPATTR audit je v index.html.');
