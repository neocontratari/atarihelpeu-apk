const fs = require('fs');
const p = 'app/src/main/assets/emu_vbxe/index.html';
const s = fs.readFileSync(p, 'utf8');
const need = [
  'BUILD2IV_GTIA_DYNAMIC_GPRIOR_SAFE',
  'gtiaRegAt(0x1B,x0)&0xC0',
  'function gtia10Color(nib,x)',
  'Postcard/G2F kernel prepina $D01B behem radky'
];
let ok = true;
for (const n of need) {
  if (!s.includes(n)) { console.error('CHYBI:', n); ok = false; }
}
if (!ok) process.exit(1);
console.log('BUILD2IV kontrola OK - dynamicky GPRIOR/GTIA mode je v index.html.');
