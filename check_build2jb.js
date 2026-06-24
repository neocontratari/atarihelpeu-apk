const fs=require('fs');
const p='app/src/main/assets/emu_vbxe/index.html';
const s=fs.readFileSync(p,'utf8');
const need=[
 'BUILD2JB_VBXE_MSEL_OVSHIFT_PRIORITY_SAFE',
 'VBXE MSEL/RGB BUILD2JB',
 'VBXE MSEL/PRIORMAP BUILD2JB',
 'vbxeOvColor',
 'ovShift',
 'BUILD2JA_XEX_LIVE_MEMAC_PAYLOAD_SAFE'
];
let ok=true;
for(const n of need){
  if(!s.includes(n)){ console.error('CHYBI:',n); ok=false; }
}
if(!ok) process.exit(1);
console.log('BUILD2JB kontrola OK - MSEL/RGB, PRIORMAP, OV_COLOR_SHIFT a BUILD2JA MEMAC cesta jsou pritomne.');
