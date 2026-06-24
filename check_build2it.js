const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const need=[
  'BUILD2IT_G2F_PMG_PRIORITY_COLOR_SAFE',
  'function pmShowAt',
  'pmColAt',
  'missileColAt',
  'GTIA COLOR/PRIOR SEG BUILD2IT',
  'new Int32Array(96)'
];
let ok=true;
for(const n of need){
  if(!s.includes(n)){ console.error('CHYBI:',n); ok=false; }
  else console.log('OK:',n);
}
if(!ok) process.exit(1);
console.log('BUILD2IT kontrola OK');
